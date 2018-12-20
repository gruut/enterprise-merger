#include "merger_client.hpp"
#include "http_client.hpp"
#include "merger_server.hpp"

namespace gruut {
void MergerClient::sendMessage(MessageType msg_type,
                               std::vector<id_type> &receiver_list,
                               std::vector<std::string> &packed_msg_list) {

  if (checkMergerMsgType(msg_type)) {
    sendToMerger(msg_type, receiver_list, packed_msg_list[0]);
  } else if (checkSignerMsgType(msg_type)) {
    sendToSigner(msg_type, receiver_list, packed_msg_list);
  } else if (checkSEMsgType(msg_type)) {
    sendToSE(packed_msg_list[0]);
  }
}

void MergerClient::sendToSE(std::string &packed_msg) {
  auto setting = Setting::getInstance();
  auto service_endpoints_list = setting->getServiceEndpointInfo();

  auto service_endpoint = service_endpoints_list[0];
  const string address =
      service_endpoint.address + ":" + service_endpoint.port + "/api/blocks";

  HttpClient http_client(address);
  http_client.post(packed_msg);
}

void MergerClient::sendToMerger(MessageType msg_type,
                                std::vector<id_type> &receiver_list,
                                std::string &packed_msg) {
  for (merger_id_type &merger_id : receiver_list) {
    // TODO: Merger ID에 따른 ip와 port를 저장해 놓을 곳 필요.
    std::unique_ptr<MergerCommunication::Stub> stub =
        MergerCommunication::NewStub(
            CreateChannel("Merger ip and port", InsecureChannelCredentials()));

    ClientContext context;

    MergerDataRequest request;
    MergerDataReply reply;

    request.set_data(packed_msg);
    Status status = stub->pushData(&context, request, &reply);
    if (!status.ok())
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
  }
}

void MergerClient::sendToSigner(MessageType msg_type,
                                std::vector<id_type> &receiver_list,
                                std::vector<std::string> &packed_msg_list) {

  int num_of_signer = receiver_list.size();

  for (int i = 0; i < num_of_signer; i++) {
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
  return (msg_type == MessageType::MSG_UP ||
          msg_type == MessageType::MSG_PING ||
          msg_type == MessageType::MSG_REQ_BLOCK ||
          msg_type == MessageType::MSG_BLOCK);
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
          msg_type == MessageType::MSG_HEADER);
  // TODO: 다른 MSG TYPE은 차 후 추가
}
}; // namespace gruut