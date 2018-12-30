#include "merger_client.hpp"
#include "../../application.hpp"
#include "http_client.hpp"
#include "merger_server.hpp"

#include "easy_logging.hpp"

using grpc::health::v1::Health;
using grpc::health::v1::HealthCheckRequest;
using grpc::health::v1::HealthCheckResponse;

namespace gruut {

MergerClient::MergerClient() {
  m_rpc_receiver_list = RpcReceiverList::getInstance();
  m_connection_list = ConnectionList::getInstance();
  m_setting = Setting::getInstance();
  m_my_id = m_setting->getMyId();

  el::Loggers::getLogger("MCLN");
}

void MergerClient::setup() {
  auto &io_service = Application::app().getIoService();
  m_conn_check_strand.reset(new boost::asio::io_service::strand(io_service));
  m_conn_check_timer.reset(new boost::asio::deadline_timer(io_service));
}

void MergerClient::checkConnection() {
  auto &io_service = Application::app().getIoService();

  io_service.post(m_conn_check_strand->wrap([this]() {
    auto merger_list = m_setting->getMergerInfo();
    auto se_list = m_setting->getServiceEndpointInfo();

    for (auto &merger_info : merger_list) {
      if (m_my_id == merger_info.id)
        continue;

      HealthCheckRequest request;
      HealthCheckResponse response;
      ClientContext context;

      std::shared_ptr<ChannelCredentials> credential =
          InsecureChannelCredentials();
      std::shared_ptr<Channel> channel = CreateChannel(
          merger_info.address + ":" + merger_info.port, credential);
      std::unique_ptr<Health::Stub> hc_stub =
          grpc::health::v1::Health::NewStub(channel);
      Status status = hc_stub->Check(&context, request, &response);

      m_connection_list->setMergerStatus(merger_info.id, status.ok());
    }

    for (auto &se_info : se_list) {
      HttpClient http_client(se_info.address + ":" + se_info.port);
      bool status = http_client.checkServStatus();

      m_connection_list->setSeStatus(se_info.id, status);
    }
  }));

  m_conn_check_timer->expires_from_now(
      boost::posix_time::seconds(config::CONN_CHECK_PERIOD));
  m_conn_check_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "MCLN") << "ConCheckTimer ABORTED";
    } else if (ec.value() == 0) {
      checkConnection();
    } else {
      CLOG(ERROR, "MCLN") << ec.message();
      // throw;
    }
  });
}

void MergerClient::sendMessage(MessageType msg_type,
                               std::vector<id_type> &receiver_list,
                               std::vector<std::string> &packed_msg_list,
                               OutputMsgEntry &output_msg) {

  // CLOG(INFO, "MCLN") << "called sendMessage()";

  if (checkMergerMsgType(msg_type)) {
    sendToMerger(receiver_list, packed_msg_list[0]);
  }

  if (checkSignerMsgType(msg_type)) {
    sendToSigner(msg_type, receiver_list, packed_msg_list);
  }

  if (checkSEMsgType(msg_type)) {
    sendToSE(receiver_list, output_msg);
  }
}

void MergerClient::sendToSE(std::vector<id_type> &receiver_list,
                            OutputMsgEntry &output_msg) {
  auto setting = Setting::getInstance();
  auto service_endpoints_list = setting->getServiceEndpointInfo();

  std::string send_msg = output_msg.body.dump();

  if (receiver_list.empty()) {
    for (auto &service_endpoint : service_endpoints_list) {
      bool status = m_connection_list->getSeStatus(service_endpoint.id);
      if (status) {
        std::string address = service_endpoint.address + ":" +
                              service_endpoint.port + "/api/blocks";
        HttpClient http_client(address);
        http_client.post(send_msg);
      }
    }
  } else {
    for (auto &receiver_id : receiver_list) {
      for (auto &service_endpoint : service_endpoints_list) {
        bool status = m_connection_list->getSeStatus(service_endpoint.id);
        if (status && service_endpoint.id == receiver_id) {
          std::string address = service_endpoint.address + ":" +
                                service_endpoint.port + "/api/blocks";
          HttpClient http_client(address);
          http_client.post(send_msg);
          break;
        }
      }
    }
  }
}

void MergerClient::sendToMerger(std::vector<id_type> &receiver_list,
                                std::string &packed_msg) {

  // CLOG(INFO, "MCLN") << "called sendToMerger()";

  MergerDataRequest request;
  request.set_data(packed_msg);

  bool sent_somewhere = false;

  if (receiver_list.empty()) {
    for (auto &merger_info : m_setting->getMergerInfo()) {
      bool status = m_connection_list->getMergerStatus(merger_info.id);
      if (status && m_my_id != merger_info.id) {
        sendMsgToMerger(merger_info, request);
        sent_somewhere = true;
      }
    }
  } else {
    for (auto &receiver_id : receiver_list) {
      for (auto &merger_info : m_setting->getMergerInfo()) {
        bool status = m_connection_list->getMergerStatus(merger_info.id);
        if (status && merger_info.id == receiver_id) {
          sendMsgToMerger(merger_info, request);
          sent_somewhere = true;
          break;
        }
      }
    }
  }

  if (!sent_somewhere) {
    CLOG(ERROR, "MCLN") << "Nowhere to send message";
  }
}

void MergerClient::sendMsgToMerger(MergerInfo &merger_info,
                                   MergerDataRequest &request) {

  CLOG(INFO, "MCLN") << "sendToMerger("
                     << merger_info.address + ":" + merger_info.port << ") ";

  std::shared_ptr<ChannelCredentials> credential = InsecureChannelCredentials();
  std::shared_ptr<Channel> channel =
      CreateChannel(merger_info.address + ":" + merger_info.port, credential);
  std::unique_ptr<MergerCommunication::Stub> stub =
      MergerCommunication::NewStub(channel);

  ClientContext context;
  MergerDataReply reply;

  Status status = stub->pushData(&context, request, &reply);
  if (!status.ok())
    CLOG(ERROR, "MCLN") << status.error_message();
}

void MergerClient::sendToSigner(MessageType msg_type,
                                std::vector<id_type> &receiver_list,
                                std::vector<std::string> &packed_msg_list) {

  size_t num_of_signer = receiver_list.size();

  for (size_t i = 0; i < num_of_signer; ++i) {
    SignerRpcInfo signer_rpc_info =
        m_rpc_receiver_list->getSignerRpcInfo(receiver_list[i]);
    switch (msg_type) {
    case MessageType::MSG_CHALLENGE: {
      auto tag = static_cast<Join *>(signer_rpc_info.tag_join);
      *signer_rpc_info.join_status = RpcCallStatus::FINISH;

      GrpcMsgChallenge reply;
      reply.set_message(packed_msg_list[i]);

      signer_rpc_info.send_challenge->Finish(reply, Status::OK, tag);
    } break;

    case MessageType::MSG_RESPONSE_2: {
      auto tag = static_cast<DHKeyEx *>(signer_rpc_info.tag_dhkeyex);
      *signer_rpc_info.dhkeyex_status = RpcCallStatus::FINISH;

      GrpcMsgResponse2 reply;
      reply.set_message(packed_msg_list[i]);

      signer_rpc_info.send_response2->Finish(reply, Status::OK, tag);
    } break;

    case MessageType::MSG_ACCEPT: {
      auto tag =
          static_cast<KeyExFinished *>(signer_rpc_info.tag_keyexfinished);
      *signer_rpc_info.keyexfinished_status = RpcCallStatus::FINISH;

      GrpcMsgAccept reply;
      reply.set_message(packed_msg_list[i]);

      signer_rpc_info.send_accept->Finish(reply, Status::OK, tag);
    } break;

    case MessageType::MSG_REQ_SSIG: {
      auto tag = static_cast<Identity *>(signer_rpc_info.tag_identity);

      GrpcMsgReqSsig reply;
      reply.set_message(packed_msg_list[i]);
      signer_rpc_info.send_req_ssig->Write(reply, tag);
    } break;

    default:
      break;
    }
  }
}

bool MergerClient::checkMergerMsgType(MessageType msg_type) {
  return (
      msg_type == MessageType::MSG_UP || msg_type == MessageType::MSG_PING ||
      msg_type == MessageType::MSG_REQ_BLOCK ||
      msg_type == MessageType::MSG_BLOCK || msg_type == MessageType::MSG_ERROR);
}

bool MergerClient::checkSignerMsgType(MessageType msg_type) {
  return (msg_type == MessageType::MSG_CHALLENGE ||
          msg_type == MessageType::MSG_RESPONSE_2 ||
          msg_type == MessageType::MSG_ACCEPT ||
          msg_type == MessageType::MSG_ECHO ||
          msg_type == MessageType::MSG_REQ_SSIG ||
          msg_type == MessageType::MSG_ERROR);
}

bool MergerClient::checkSEMsgType(MessageType msg_type) {
  return (msg_type == MessageType::MSG_UP ||
          msg_type == MessageType::MSG_PING ||
          msg_type == MessageType::MSG_HEADER ||
          msg_type == MessageType::MSG_ERROR);
}
}; // namespace gruut