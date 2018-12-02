#include "merger_server.hpp"
#include "message_handler.hpp"
#include <chrono>
#include <cstring>
#include <future>
#include <thread>
namespace gruut {

void MergerServer::runServer(char const *port) {
  std::string port_num(port);
  std::string server_address("0.0.0.0:");
  server_address += port_num;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&m_merger_service);
  builder.RegisterService(&m_se_service);
  builder.RegisterService(&m_signer_service);
  m_completion_queue = builder.AddCompletionQueue();
  m_server = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;

  recvData();
}

void MergerServer::recvData() {
  new RecvFromMerger(&m_merger_service, m_completion_queue.get());
  new RecvFromSE(&m_se_service, m_completion_queue.get());
  new OpenChannel(&m_signer_service, m_completion_queue.get());
  new Join(&m_signer_service, m_completion_queue.get());
  new DHKeyEx(&m_signer_service, m_completion_queue.get());
  new KeyExFinished(&m_signer_service, m_completion_queue.get());
  new SigSend(&m_signer_service, m_completion_queue.get());

  void *tag;
  bool ok;
  while (true) {
    std::this_thread::sleep_for(chrono::milliseconds(200));
    GPR_ASSERT(m_completion_queue->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallData *>(tag)->proceed();
  }
}

void MergerServer::RecvFromMerger::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestpushData(&m_context, &m_request, &m_responder,
                               m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new RecvFromMerger(m_service, m_completion_queue);

    std::string packed_msg = m_request.data();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();
    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();
    MergerDataReply m_reply;
    m_receive_status = CallStatus::FINISH;
    m_responder.Finish(m_reply, final_rpc_status, this);
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::RecvFromSE::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->Requesttransaction(&m_context, &m_request, &m_responder,
                                  m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new RecvFromSE(m_service, m_completion_queue);

    std::string packed_msg = m_request.message();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();
    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();
    Nothing m_reply;
    m_receive_status = CallStatus::FINISH;
    m_responder.Finish(m_reply, final_rpc_status, this);
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::OpenChannel::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestopenChannel(&m_context, m_stream.get(),
                                  m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new OpenChannel(m_service, m_completion_queue);

    m_stream->Read(&m_request, this);
    uint64_t receiver_id;
    std::memcpy(&receiver_id, &m_request.sender()[0], sizeof(uint64_t));
    // TODO: receiver_id_list 에 map[receiver_id].stream = m_stream 저장

    m_receive_status = CallStatus::FINISH;
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::Join::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->Requestjoin(&m_context, &m_request, m_responder.get(),
                           m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new Join(m_service, m_completion_queue);

    std::string packed_msg = m_request.message();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();

    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();
    if (!final_rpc_status.ok()) {
      GrpcMsgChallenge m_reply;
      m_responder->Finish(m_reply, final_rpc_status, this);
    }
    m_receive_status = CallStatus::FINISH;
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::DHKeyEx::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestdhKeyEx(&m_context, &m_request, m_responder.get(),
                              m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new DHKeyEx(m_service, m_completion_queue);

    std::string packed_msg = m_request.message();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();

    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();

    if (!final_rpc_status.ok()) {
      GrpcMsgResponse2 m_reply;
      m_responder->Finish(m_reply, final_rpc_status, this);
    }
    m_receive_status = CallStatus::FINISH;
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::KeyExFinished::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestkeyExFinished(&m_context, &m_request, m_responder.get(),
                                    m_completion_queue, m_completion_queue,
                                    this);
  } break;

  case CallStatus::PROCESS: {
    new KeyExFinished(m_service, m_completion_queue);

    std::string packed_msg = m_request.message();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();

    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();
    if (!final_rpc_status.ok()) {
      GrpcMsgAccept m_reply;
      m_responder->Finish(m_reply, final_rpc_status, this);
    }
    m_receive_status = CallStatus::FINISH;
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

void MergerServer::SigSend::proceed() {
  switch (m_receive_status) {
  case CallStatus::CREATE: {
    m_receive_status = CallStatus::PROCESS;
    m_service->RequestsigSend(&m_context, &m_request, &m_responder,
                              m_completion_queue, m_completion_queue, this);
  } break;

  case CallStatus::PROCESS: {
    new SigSend(m_service, m_completion_queue);

    std::string packed_msg = m_request.message();
    std::promise<Status> rpc_status;
    std::future<Status> future_rpc_status = rpc_status.get_future();
    Status final_rpc_status = future_rpc_status.get();

    MessageHandler message_handler;
    std::thread handler(&MessageHandler::unpackMsg, &message_handler,
                        std::ref(packed_msg), std::ref(rpc_status));
    handler.join();
    NoReply m_reply;
    m_responder.Finish(m_reply, final_rpc_status, this);
    m_receive_status = CallStatus::FINISH;
  } break;

  default: {
    GPR_ASSERT(m_receive_status == CallStatus::FINISH);
    delete this;
  } break;
  }
}

} // namespace gruut