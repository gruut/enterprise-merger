#include "merger_server.hpp"
#include "message_handler.hpp"
#include "rpc_receiver_list.hpp"
#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <thread>
namespace gruut {

void MergerServer::runServer(const std::string &port_num) {
  std::string server_address("0.0.0.0:");
  server_address += port_num;
  m_port_num = port_num;
  ServerBuilder builder;

  EnableDefaultHealthCheckService(true);
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&m_merger_service);
  builder.RegisterService(&m_se_service);
  builder.RegisterService(&m_signer_service);
  m_completion_queue = builder.AddCompletionQueue();
  m_server = builder.BuildAndStart();

  HealthCheckServiceInterface *hc_service = m_server->GetHealthCheckService();
  hc_service->SetServingStatus("healthy_service", true);

  m_is_started = true;

  CLOG(INFO, "MSVR") << "Server listening on " << server_address;

  new RecvFromMerger(&m_merger_service, m_completion_queue.get());
  new RecvFromSE(&m_se_service, m_completion_queue.get());
  new OpenChannel(&m_signer_service, m_completion_queue.get());
  new Join(&m_signer_service, m_completion_queue.get());
  new DHKeyEx(&m_signer_service, m_completion_queue.get());
  new KeyExFinished(&m_signer_service, m_completion_queue.get());
  new SigSend(&m_signer_service, m_completion_queue.get());

  recvMessage();
}

void MergerServer::recvMessage() {
  void *tag;
  bool ok;
  try {
    while (true) {
      if (m_input_queue->size() < config::AVAILABLE_INPUT_SIZE) {
        if (m_completion_queue->Next(&tag, &ok)) {
          if (ok)
            static_cast<CallData *>(tag)->proceed();
        }
      } else {
        CLOG(INFO, "MSVR") << "#InputQueue = " << m_input_queue->size();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    }
  } catch (std::exception &e) {
    CLOG(ERROR, "MSVR") << "RPC Server problem : " << e.what();
    while (m_completion_queue->Next(&tag, &ok)) {
      if (tag != nullptr)
        static_cast<CallData *>(tag)->proceed(false);
    }
    m_completion_queue.reset();
    m_server.reset();
    CLOG(INFO, "MSVR") << "RPC Server restart";
    runServer(m_port_num);
  }
}

void RecvFromMerger::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestpushData(&m_context, &m_request, &m_responder,
                               m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new RecvFromMerger(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      std::string packed_msg = m_request.data();
      Status rpc_status;
      id_type recv_id;

      MessageHandler message_handler;
      message_handler.unpackMsg(packed_msg, rpc_status, recv_id);

      ConnectionList::getInstance()->setMergerStatus(recv_id, true);

      MergerDataReply m_reply;
      m_receive_status = RpcCallStatus::FINISH;
      m_responder.Finish(m_reply, rpc_status, this);
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "RecvFromMerger() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}

void RecvFromSE::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }

  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->Requesttransaction(&m_context, &m_request, &m_responder,
                                  m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new RecvFromSE(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      Status rpc_status;
      servend_id_type receiver_id;
      TxReply m_reply;
      try {
        std::string packed_msg = m_request.message();
        MessageHandler message_handler;
        message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);

        if (rpc_status.ok()) {
          m_reply.set_status(TxReply_Status_SUCCESS);
          m_reply.set_message("OK");
        } else {
          m_reply.set_status(TxReply_Status_INVALID);
          m_reply.set_message(rpc_status.error_message());
        }
      } catch (std::exception &e) {
        m_reply.set_status(TxReply_Status_INTERNAL);
        m_reply.set_message("Merger internal error");
        CLOG(INFO, "MSVR") << e.what();
      }
      m_receive_status = RpcCallStatus::FINISH;
      m_responder.Finish(m_reply, rpc_status, this);
    });
  } break;
  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "RecvFromSE() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}

void OpenChannel::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::READ;
    m_context.AsyncNotifyWhenDone(this);
    m_service->RequestopenChannel(&m_context, &m_stream, m_completion_queue,
                                  m_completion_queue, this);
  } break;

  case RpcCallStatus::READ: {
    new OpenChannel(m_service, m_completion_queue);
    m_receive_status = RpcCallStatus::PROCESS;
    m_stream.Read(&m_request, this);
  } break;

  case RpcCallStatus::PROCESS: {
    id_type receiver_id;

    m_signer_id_b64 = m_request.sender();
    receiver_id = TypeConverter::decodeBase64(m_signer_id_b64);
    m_receive_status = RpcCallStatus::WAIT;
    m_rpc_receiver_list->setReqSsig(receiver_id, &m_stream, this);

  } break;

  case RpcCallStatus::WAIT: {
    if (m_context.IsCancelled()) {
      CLOG(INFO, "MSVR") << "Disconnected with signer (" << m_signer_id_b64
                         << ")";
      MessageHandler msg_handler;
      msg_handler.genInternalMsg(MessageType::MSG_LEAVE, m_signer_id_b64);
      delete this;
    }
  } break;

  default:
    break;
  }
}

void Join::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->Requestjoin(&m_context, &m_request, &m_responder,
                           m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new Join(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      std::string packed_msg = m_request.message();

      Status rpc_status;
      id_type receiver_id;

      MessageHandler message_handler;
      message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);

      if (!rpc_status.ok()) {
        GrpcMsgChallenge m_reply;
        m_responder.Finish(m_reply, rpc_status, this);
        m_receive_status = RpcCallStatus::FINISH;
      } else {
        m_rpc_receiver_list->setChanllenge(receiver_id, &m_responder, this,
                                           &m_receive_status);
      }
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "Join() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}

void DHKeyEx::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestdhKeyEx(&m_context, &m_request, &m_responder,
                              m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new DHKeyEx(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      std::string packed_msg = m_request.message();
      Status rpc_status;
      id_type receiver_id;

      MessageHandler message_handler;
      message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);

      if (!rpc_status.ok()) {
        GrpcMsgResponse2 m_reply;
        m_responder.Finish(m_reply, rpc_status, this);
      } else {
        m_rpc_receiver_list->setResponse2(receiver_id, &m_responder, this,
                                          &m_receive_status);
      }
      m_receive_status = RpcCallStatus::FINISH;
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "DHKeyEx() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}

void KeyExFinished::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestkeyExFinished(&m_context, &m_request, &m_responder,
                                    m_completion_queue, m_completion_queue,
                                    this);
  } break;

  case RpcCallStatus::PROCESS: {
    new KeyExFinished(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      std::string packed_msg = m_request.message();
      Status rpc_status;
      id_type receiver_id;

      MessageHandler message_handler;
      message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);
      if (!rpc_status.ok()) {
        GrpcMsgAccept m_reply;
        m_responder.Finish(m_reply, rpc_status, this);
      } else {
        m_rpc_receiver_list->setAccept(receiver_id, &m_responder, this,
                                       &m_receive_status);
      }
      m_receive_status = RpcCallStatus::FINISH;
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "KeyExFinished() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}

void SigSend::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestsigSend(&m_context, &m_request, &m_responder,
                              m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new SigSend(m_service, m_completion_queue);

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      std::string packed_msg = m_request.message();
      Status rpc_status;
      id_type receiver_id;

      MessageHandler message_handler;
      message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);
      NoReply m_reply;
      m_responder.Finish(m_reply, rpc_status, this);
      m_receive_status = RpcCallStatus::FINISH;
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "SigSend() rpc status : Unknown";
    }
    delete this;
  } break;
  }
}
} // namespace gruut