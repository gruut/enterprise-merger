#include "grpc_merger.hpp"
#include "../../utils/compressor.hpp"
#include "grpc_util.hpp"

namespace gruut {

void MergerRpcServer::runMergerServ(char *port_for_merger) {
  std::string port_num(port_for_merger);
  std::string server_address("0.0.0.0:");
  server_address += port_num;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&m_service_merger);
  m_cq_merger = builder.AddCompletionQueue();
  m_server_merger = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << " for Merger"
            << std::endl;
  handleMergerRpcs();
}
void MergerRpcServer::runSignerServ(char *port_for_signer) {
  std::string port_num(port_for_signer);
  std::string server_address("0.0.0.0:");
  server_address += port_num;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&m_service_signer);
  m_cq_signer = builder.AddCompletionQueue();
  m_server_signer = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << " for Signer"
            << std::endl;
  handleSignerRpcs();
}

void MergerRpcServer::ReceiveData::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestpushData(&m_ctx, &m_request, &m_responder, m_cq, m_cq,
                               this);
  } break;

  case CallStatus::PROCESS: {
    new ReceiveData(m_service, m_cq);

    Status st;
    std::string raw_data = m_request.data();
    MessageHeader msg_header = HeaderController::parseHeader(raw_data);
    if (!HeaderController::validateMessage(msg_header)) {
      m_reply.set_checker(false);
    } else {
      int json_size = HeaderController::getJsonSize(msg_header);
      std::string compressed_data = HeaderController::detachHeader(raw_data);
      std::string decompressed_data;
      // TODO: MAC verification 필요

      Compressor::decompressData(compressed_data, decompressed_data, json_size);

      Message msg(msg_header);
      msg.data = nlohmann::json::parse(decompressed_data);

      if (JsonValidator::validateSchema(msg.data, msg.message_type)) {
        auto input_queue = Application::app().getInputQueue();
        input_queue->push(msg);
        m_reply.set_checker(true);
        st = Status::OK;
      } else {
        m_reply.set_checker(false);
        st = Status(grpc::StatusCode::CANCELLED, "Json schema check fail");
      }
    }
    m_receive_status = CallStatus::FINISH;
    m_responder.Finish(m_reply, st, this);
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerRpcServer::PullRequest::proceed() {
  switch (m_pull_status) {
  case CallStatus::CREATE: {
    m_pull_status = CallStatus::PROCESS;
    m_pull_service->RequestpullData(&m_pull_ctx, &m_pull_request,
                                    &m_pull_responder, m_pull_cq, m_pull_cq,
                                    this);
  } break;

  case CallStatus::PROCESS: {
    new PullRequest(m_pull_service, m_pull_cq);

    m_pull_reply.set_message("someData");
    m_pull_status = CallStatus::FINISH;
    m_pull_responder.Finish(m_pull_reply, Status::OK, this);
  } break;

  default: {
    GPR_ASSERT(m_pull_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerRpcServer::handleMergerRpcs() {
  new ReceiveData(&m_service_merger, m_cq_merger.get());
  void *tag;
  bool ok;
  while (true) {
    GPR_ASSERT(m_cq_merger->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallData *>(tag)->proceed();
  }
}
void MergerRpcServer::handleSignerRpcs() {
  new PullRequest(&m_service_signer, m_cq_signer.get());
  void *tag;
  bool ok;
  while (true) {
    GPR_ASSERT(m_cq_signer->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallData *>(tag)->proceed();
  }
}

void MergerRpcClient::run() {
  auto output_queue = Application::app().getOutputQueue();
  // TODO: 아래 if문을 포함하는 무한루프 필요
  if (!output_queue->empty()) {
    Message msg = output_queue->front();
    if (checkMsgType(msg.message_type)) {
      output_queue->pop();
      std::string compressed_json;
      std::string json_dump = msg.data.dump();
      Compressor::compressData(json_dump, compressed_json);
      std::string header_added_data = HeaderController::attachHeader(
          compressed_json, msg.message_type, msg.mac_algo_type,
          msg.compression_algo_type);

      // TODO: MAC 붙이는 것 필요

      sendData(header_added_data);
    }
  }
}

bool MergerRpcClient::pushData(
    std::string &compressed_data,
    std::unique_ptr<MergerCommunication::Stub> stub) {
  MergerDataRequest request;
  request.set_data(compressed_data);

  MergerDataReply reply;
  ClientContext context;
  Status status = stub->pushData(&context, request, &reply);

  if (status.ok())
    return reply.checker();
  else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return false;
  }
}

bool MergerRpcClient::checkMsgType(MessageType msg_type) {
  return (msg_type == MessageType::MSG_ECHO ||
          msg_type == MessageType::MSG_BLOCK);
}

void MergerRpcClient::sendData(std::string &header_added_data) {
  // TODO: 현재는 로컬호스트로 받는곳 지정 해놓음 변경 필요.
  std::unique_ptr<MergerCommunication::Stub> stub =
      MergerCommunication::NewStub(grpc::CreateChannel(
          "localhost:50051", grpc::InsecureChannelCredentials()));

  std::thread th([&]() { pushData(header_added_data, move(stub)); });
}
} // namespace gruut