#include "merger_client.hpp"

namespace gruut {
void MergerClient::sendMessage(MessageType msg_type,
                               std::vector<uint64_t> &receiver_list,
                               std::string &msg) {

  if (checkMergerMsgType(msg_type)) {
    sendToMerger(msg_type, receiver_list, msg);
  } else if (checkSignerMsgType(msg_type)) {
    sendToSigner(msg_type, receiver_list, msg);
  }

  if (checkSEMsgType(msg_type)) {
    sendToSE(msg_type, receiver_list, msg);
  }
}

void MergerClient::sendToSE(MessageType msg_type,
                            std::vector<uint64_t> &receiver_list,
                            std::string &msg) {
  for (uint64_t se_id : receiver_list) {
    // TODO: SE ID에 따른 ip와 port를 저장해 놓을 곳 필요.
    std::unique_ptr<GruutSeService::Stub> stub = GruutSeService::NewStub(
        CreateChannel("SE ip and port", InsecureChannelCredentials()));

    ClientContext context;
    // TODO: SE protobuf 수정작업 후 주석 해제.

    /*  DataRequest request;
      DataReply reply;

      request.set_data(msg);
      Status status = stub->sendData(&context, request, &reply);
      if(!status.ok())
        std::cout<<status.error_code() << ":
    "<<status.error_message()<<std::endl;
    }*/
  }
}

void MergerClient::sendToMerger(MessageType msg_type,
                                std::vector<uint64_t> &receiver_list,
                                std::string &msg) {
  for (uint64_t merger_id : receiver_list) {
    // TODO: Merger ID에 따른 ip와 port를 저장해 놓을 곳 필요.
    std::unique_ptr<MergerCommunication::Stub> stub =
        MergerCommunication::NewStub(
            CreateChannel("Merger ip and port", InsecureChannelCredentials()));

    ClientContext context;

    MergerDataRequest request;
    MergerDataReply reply;

    request.set_data(msg);
    Status status = stub->pushData(&context, request, &reply);
    if (!status.ok())
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
  }
}

void MergerClient::sendToSigner(MessageType msg_type,
                                std::vector<uint64_t> &receiver_list,
                                std::string &msg) {
  for (uint64_t signer_id : receiver_list) {
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
  return (msg_type == MessageType::MSG_UP || msg_type == MessageType::MSG_PING);
  // TODO: 다른 MSG TYPE은 차 후 추가
}
}; // namespace gruut