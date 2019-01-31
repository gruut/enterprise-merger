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

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&m_merger_service);
  builder.RegisterService(&m_se_service);
  builder.RegisterService(&m_signer_service);
  m_completion_queue = builder.AddCompletionQueue();
  m_server = builder.BuildAndStart();

  m_is_started = true;

  CLOG(INFO, "MSVR") << "Server listening on " << server_address;

  new CheckConn(&m_merger_service, m_completion_queue.get());

  new RecvFromMerger(&m_merger_service, m_completion_queue.get());
  new RecvFromSE(&m_se_service, m_completion_queue.get());
  new OpenChannel(&m_signer_service, m_completion_queue.get());
  new SignerService(&m_signer_service, m_completion_queue.get());

  recvMessage();
}

void MergerServer::recvMessage() {
  void *tag;
  bool ok;
  try {
    while (true) {
      if (m_input_queue->size() < config::AVAILABLE_INPUT_SIZE) {
        GPR_ASSERT(m_completion_queue->Next(&tag, &ok));
        if (ok)
          static_cast<CallData *>(tag)->proceed();
      } else {
        // CLOG(INFO, "MSVR") << "#InputQueue = " << m_input_queue->size();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    }
  } catch (std::exception &e) {
    CLOG(ERROR, "MSVR") << "RPC Server problem : " << e.what();
    if (m_completion_queue != nullptr) {
      while (m_completion_queue->Next(&tag, &ok)) {
        if (tag != nullptr)
          static_cast<CallData *>(tag)->proceed(false);
      }
    }
    m_completion_queue.reset();
    m_server.reset();
    CLOG(INFO, "MSVR") << "RPC Server restart";
    runServer(m_port_num);
  }
}

void CheckConn::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestConnCheck(&m_context, &m_request, &m_responder,
                                m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new CheckConn(m_service, m_completion_queue);
    ConnCheckResponse m_reply;
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "RecvFromMerger() rpc status : Unknown";
    }
    delete this;
  } break;
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

    std::async(std::launch::async, [this]() {
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
    m_service->RequestseService(&m_context, &m_request, &m_responder,
                                m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new RecvFromSE(m_service, m_completion_queue);

    std::async(std::launch::async, [this]() {
      Status rpc_status;
      servend_id_type receiver_id;
      Reply m_reply;
      try {
        std::string packed_msg = m_request.message();
        MessageHandler message_handler;
        message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);

        if (rpc_status.ok()) {
          m_reply.set_status(Reply_Status_SUCCESS);
          m_reply.set_message("OK");
        } else {
          m_reply.set_status(Reply_Status_INVALID);
          m_reply.set_message(rpc_status.error_message());
        }
      } catch (std::exception &e) {
        m_reply.set_status(Reply_Status_INTERNAL);
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
    m_signer_id_b64 = m_request.sender();
    m_receive_status = RpcCallStatus::WAIT;
    m_rpc_receiver_list->setReplyMsg(m_signer_id_b64, &m_stream, this);

  } break;

  case RpcCallStatus::WAIT: {
    if (m_context.IsCancelled()) {
      CLOG(INFO, "MSVR") << "Disconnected with signer (" << m_signer_id_b64
                         << ")";
      MessageHandler msg_handler;
      msg_handler.genInternalMsg(MessageType::MSG_LEAVE, m_signer_id_b64);
      m_rpc_receiver_list->eraseRpcInfo(m_signer_id_b64);
      delete this;
    }
  } break;

  default:
    break;
  }
}

void SignerService::proceed(bool st) {
  if (!st) {
    delete this;
    return;
  }
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestsignerService(&m_context, &m_request, &m_responder,
                                    m_completion_queue, m_completion_queue,
                                    this);
  } break;

  case RpcCallStatus::PROCESS: {
    new SignerService(m_service, m_completion_queue);

    std::async(std::launch::async, [this]() {
      Status rpc_status;
      id_type receiver_id;
      MsgStatus m_reply;
      try {
        std::string packed_msg = m_request.message();
        MessageHandler message_handler;
        message_handler.unpackMsg(packed_msg, rpc_status, receiver_id);

        if (rpc_status.ok()) {
          m_reply.set_status(MsgStatus_Status_SUCCESS);
          m_reply.set_message("OK");
        } else {
          m_reply.set_status(MsgStatus_Status_INVALID);
          m_reply.set_message(rpc_status.error_message());
        }
      } catch (std::exception &e) {
        m_reply.set_status(MsgStatus_Status_INTERNAL);
        m_reply.set_message("Merger internal error");
        CLOG(INFO, "MSVR") << e.what();
      }
      m_receive_status = RpcCallStatus::FINISH;
      m_responder.Finish(m_reply, rpc_status, this);
    });
  } break;

  default: {
    if (m_receive_status != RpcCallStatus::FINISH) {
      CLOG(ERROR, "MSVR") << "Signer rpc status : Unknown";
    }
    delete this;
  } break;
  }
}
} // namespace gruut