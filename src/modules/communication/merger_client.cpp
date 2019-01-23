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
  m_conn_manager = ConnManager::getInstance();

  el::Loggers::getLogger("MCLN");
}

void MergerClient::accessToTracker() {
  auto setting = Setting::getInstance();
  std::string my_id_b64 = TypeConverter::encodeBase64(setting->getMyId());
  json request_msg;
  request_msg["mID"] = my_id_b64;
  request_msg["ip"] = setting->getMyAddress();
  request_msg["port"] = setting->getMyPort();
  request_msg["mCert"] = setting->getMyCert();
  request_msg["time"] = Time::now();
  // TODO: 아래는 현재 테스트를 위한 값으로 수정 될 것.
  request_msg["hgt"] = to_string(0);
  request_msg["bID"] = to_string(0);
  request_msg["hash"] = to_string(0);
  request_msg["prevHash"] = to_string(0);
  request_msg["prevbID"] = to_string(0);

  // ToDO: 현재 local주소, setting에 tracker 정보 세팅 필요.
  CLOG(INFO, "MCLN") << "ACCESS TO TRACKER";
  json response_msg;
  HttpClient http_client("ec2-13-125-221-27.ap-northeast-2.compute.amazonaws."
                         "com/src/JoinMerger.php");
  CURLcode status =
      http_client.postAndGetReply(request_msg.dump(), response_msg);

  if (status != CURLE_OK) {
    CLOG(ERROR, "MCLN") << "Could not get Merger informations from Tracker";
    return;
  }

  if (response_msg.find("merger") != response_msg.end() &&
      response_msg.find("se") != response_msg.end()) {

    for (auto &merger : response_msg["merger"]) {
      string merger_id_b64 = Safe::getString(merger, "mID");
      merger_id_type merger_id = TypeConverter::decodeBase64(merger_id_b64);
      auto block_height = Safe::getInt(merger, "hgt");
      m_conn_manager->setMergerBlockHgt(merger_id, block_height);

      if (m_conn_manager->hasMergerInfo(merger_id) ||
          merger_id_b64 == my_id_b64)
        continue;

      MergerInfo merger_info;
      merger_info.id = merger_id;
      merger_info.address = Safe::getString(merger, "ip");
      merger_info.port = Safe::getString(merger, "port");
      merger_info.cert = Safe::getString(merger, "mCert");

      m_conn_manager->setMergerInfo(merger_info, true);
    }

    for (auto &se : response_msg["se"]) {
      ServiceEndpointInfo se_info;
      string se_id_b64 = Safe::getString(se, "seID");
      servend_id_type se_id = TypeConverter::decodeBase64(se_id_b64);

      if (m_conn_manager->hasSeInfo(se_id))
        continue;

      se_info.id = se_id;
      se_info.address = Safe::getString(se, "ip");
      se_info.port = Safe::getString(se, "port");
      se_info.cert = Safe::getString(se, "seCert");

      m_conn_manager->setSeInfo(se_info, true);
    }
  }

  auto merger_list = m_conn_manager->getAllMergerInfo();
  for (auto &merger_info : merger_list) {
    Status st = sendHealthCheck(merger_info);
    if (st.ok()) {
      auto &bp_scheduler = Application::app().getBpScheduler();
      bp_scheduler.setWelcome(false);
      return;
    }
  }
}

void MergerClient::setup() {
  auto &io_service = Application::app().getIoService();
  m_rpc_check_strand.reset(new boost::asio::io_service::strand(io_service));
  m_rpc_check_timer.reset(new boost::asio::deadline_timer(io_service));

  m_http_check_strand.reset(new boost::asio::io_service::strand(io_service));
  m_http_check_timer.reset(new boost::asio::deadline_timer(io_service));
}

void MergerClient::checkHttpConnection() {
  auto &io_service = Application::app().getIoService();

  io_service.post(m_http_check_strand->wrap([this]() {
    auto se_list = m_conn_manager->getAllSeInfo();

    for (auto &se_info : se_list) {
      HttpClient http_client(se_info.address + ":" + se_info.port);
      bool status = http_client.checkServStatus();

      m_conn_manager->setSeStatus(se_info.id, status);
    }
  }));

  m_http_check_timer->expires_from_now(
      boost::posix_time::seconds(config::HTTP_CHECK_INTERVAL));
  m_http_check_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "MCLN") << "HttpConnCheckTimer ABORTED";
    } else if (ec.value() == 0) {
      checkHttpConnection();
    } else {
      CLOG(ERROR, "MCLN") << ec.message();
      // throw;
    }
  });
}

void MergerClient::checkRpcConnection() {
  auto &io_service = Application::app().getIoService();

  io_service.post(m_rpc_check_strand->wrap([this]() {
    auto merger_list = m_conn_manager->getAllMergerInfo();
    for (auto &merger_info : merger_list) {

      Status st = sendHealthCheck(merger_info);
      m_conn_manager->setMergerStatus(merger_info.id, st.ok());
    }
  }));

  m_rpc_check_timer->expires_from_now(
      boost::posix_time::seconds(config::RPC_CHECK_INTERVAL));
  m_rpc_check_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "MCLN") << "RpcConnCheckTimer ABORTED";
    } else if (ec.value() == 0) {
      checkRpcConnection();
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
    auto api_path = getApiPath(msg_type);
    sendToSE(receiver_list, output_msg, api_path);
  }

  if (checkTrackerMsgType(msg_type)) {
    sendToTracker(output_msg);
  }
}

json MergerClient::sendToTracker(OutputMsgEntry &output_msg) {
  std::string send_msg = output_msg.body.dump();
  // TODO : setting에서 tracker 정보 받아 올 수 있도록 수정 필요.
  std::string address =
      "ec2-13-125-221-27.ap-northeast-2.compute.amazonaws.com/src/";

  json reply_json;
  if (output_msg.type == MessageType::MSG_CHAIN_INFO) {
    address += "ChainInfo.php";
    HttpClient http_client(address);
    http_client.post(send_msg);
  } else {
    address += "CheckExist.php";
    HttpClient http_client(address);
    http_client.postAndGetReply(send_msg, reply_json);
  }
  return reply_json;
}
void MergerClient::sendToSE(std::vector<id_type> &receiver_list,
                            OutputMsgEntry &output_msg, std::string api_path) {

  std::string send_msg = output_msg.body.dump();

  if (receiver_list.empty()) {
    auto service_endpoints_list = m_conn_manager->getAllSeInfo();

    for (auto &service_endpoint : service_endpoints_list) {
      if (service_endpoint.conn_status) {
        std::string address =
            service_endpoint.address + ":" + service_endpoint.port + api_path;
        HttpClient http_client(address);
        http_client.post(send_msg);
      }
    }
  } else {
    for (auto &receiver_id : receiver_list) {
      auto service_endpoint = m_conn_manager->getSeInfo(receiver_id);
      if (service_endpoint.conn_status) {
        std::string address =
            service_endpoint.address + ":" + service_endpoint.port + api_path;
        HttpClient http_client(address);
        http_client.post(send_msg);
        break;
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
    auto merger_list = m_conn_manager->getAllMergerInfo();
    for (auto &merger_info : merger_list) {
      if (merger_info.conn_status) {
        sendMsgToMerger(merger_info, request);
        sent_somewhere = true;
      }
    }
  } else {
    for (auto &receiver_id : receiver_list) {

      if (!m_conn_manager->hasMergerInfo(receiver_id)) {
        std::string receiver_id_b64 = TypeConverter::encodeBase64(receiver_id);
        OutputMsgEntry output_msg;
        json body;
        body["mID"] = receiver_id_b64;
        output_msg.body = body;

        json reply_json = sendToTracker(output_msg);

        if (reply_json.find("flag") != reply_json.end()) {
          CLOG(ERROR, "MCLN")
              << "Tracker has no information about (" << receiver_id_b64 << ")";
          continue;
        }
        MergerInfo merger_info;
        string m_id_b64 = Safe::getString(reply_json, "mID");

        merger_info.id = receiver_id;
        merger_info.address = Safe::getString(reply_json, "ip");
        merger_info.port = Safe::getString(reply_json, "port");
        merger_info.cert = Safe::getString(reply_json, "mCert");
        m_conn_manager->setMergerInfo(merger_info, true);
      }
      MergerInfo merger_info = m_conn_manager->getMergerInfo(receiver_id);
      if (merger_info.conn_status) {
        sendMsgToMerger(merger_info, request);
        sent_somewhere = true;
        break;
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
    CLOG(ERROR, "MCLN") << "Opponent's response - " << status.error_message();
}

void MergerClient::sendToSigner(MessageType msg_type,
                                std::vector<id_type> &receiver_list,
                                std::vector<std::string> &packed_msg_list) {

  size_t num_of_signer = receiver_list.size();

  for (size_t i = 0; i < num_of_signer; ++i) {
    SignerRpcInfo signer_rpc_info =
        m_rpc_receiver_list->getSignerRpcInfo(receiver_list[i]);

    auto tag = static_cast<Identity *>(signer_rpc_info.tag_identity);
    ReplyMsg reply;
    reply.set_message(packed_msg_list[i]);
    if (signer_rpc_info.send_msg != nullptr)
      signer_rpc_info.send_msg->Write(reply, tag);
  }
}

Status MergerClient::sendHealthCheck(MergerInfo &merger_info) {
  HealthCheckRequest request;
  request.set_service("healthy_service");
  HealthCheckResponse response;
  ClientContext context;

  std::shared_ptr<ChannelCredentials> credential = InsecureChannelCredentials();
  std::shared_ptr<Channel> channel =
      CreateChannel(merger_info.address + ":" + merger_info.port, credential);
  std::unique_ptr<Health::Stub> hc_stub = health::v1::Health::NewStub(channel);

  Status st = hc_stub->Check(&context, request, &response);
  return st;
}

bool MergerClient::checkMergerMsgType(MessageType msg_type) {
  // clang-format off
  return (
      msg_type == MessageType::MSG_UP ||
      msg_type == MessageType::MSG_PING ||
      msg_type == MessageType::MSG_REQ_BLOCK ||
      msg_type == MessageType::MSG_WELCOME ||
      msg_type == MessageType::MSG_REQ_STATUS ||
      msg_type == MessageType::MSG_RES_STATUS ||
      msg_type == MessageType::MSG_BLOCK ||
      msg_type == MessageType::MSG_ERROR
      );
  // clang-format on
}

bool MergerClient::checkSignerMsgType(MessageType msg_type) {
  // clang-format off
  return (
      msg_type == MessageType::MSG_CHALLENGE ||
      msg_type == MessageType::MSG_RESPONSE_2 ||
      msg_type == MessageType::MSG_ACCEPT ||
      msg_type == MessageType::MSG_REQ_SSIG ||
      msg_type == MessageType::MSG_ERROR
      );
  // clang-format on
}

bool MergerClient::checkSEMsgType(MessageType msg_type) {
  // clang-format off
  return (
      msg_type == MessageType::MSG_UP ||
      msg_type == MessageType::MSG_PING ||
      msg_type == MessageType::MSG_HEADER ||
      msg_type == MessageType::MSG_RES_CHECK ||
      msg_type == MessageType::MSG_ERROR
      );
  // clang-format on
}

bool MergerClient::checkTrackerMsgType(gruut::MessageType msg_type) {
  return (msg_type == MessageType::MSG_CHAIN_INFO);
}

std::string MergerClient::getApiPath(MessageType msg_type) {
  std::string path;

  switch (msg_type) {
  case MessageType::MSG_PING:
    path = "/api/ping";
    break;
  case MessageType::MSG_HEADER:
    path = "/api/blocks";
    break;
  default:
    break;
  }

  return path;
}
}; // namespace gruut