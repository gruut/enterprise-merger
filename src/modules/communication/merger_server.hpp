#ifndef GRUUT_ENTERPRISE_MERGER_MERGER_SERVER_HPP
#define GRUUT_ENTERPRISE_MERGER_MERGER_SERVER_HPP

#include "../../services/input_queue.hpp"
#include "protos/health.grpc.pb.h"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <atomic>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>
#include <memory>

#include "easy_logging.hpp"

using namespace grpc;
using namespace grpc_merger;
using namespace grpc_se;
using namespace grpc_signer;

namespace gruut {

class MergerServer {
public:
  MergerServer() {
    m_input_queue = InputQueueAlt::getInstance();
    el::Loggers::getLogger("MSVR");
  }
  ~MergerServer() {
    m_server->Shutdown();
    m_completion_queue->Shutdown();
  }
  void runServer(const std::string &port_num);

  inline bool isStarted() { return m_is_started; }

private:
  std::string m_port_num;
  std::unique_ptr<Server> m_server;
  std::unique_ptr<ServerCompletionQueue> m_completion_queue;
  MergerCommunication::AsyncService m_merger_service;
  GruutSeService::AsyncService m_se_service;
  GruutSignerService::AsyncService m_signer_service;
  InputQueueAlt *m_input_queue;
  void recvMessage();
  std::atomic<bool> m_is_started{false};
};

class CallData {
public:
  virtual void proceed(bool st = true) = 0;

protected:
  ServerCompletionQueue *m_completion_queue;
  ServerContext m_context;
  RpcCallStatus m_receive_status;
};

class RecvFromMerger final : public CallData {
public:
  RecvFromMerger(MergerCommunication::AsyncService *service,
                 ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    proceed();
  }
  void proceed(bool st = true);

private:
  MergerCommunication::AsyncService *m_service;
  MergerDataRequest m_request;
  ServerAsyncResponseWriter<MergerDataReply> m_responder;
};

class RecvFromSE final : public CallData {
public:
  RecvFromSE(GruutSeService::AsyncService *service, ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    proceed();
  }
  void proceed(bool st = true);

private:
  GruutSeService::AsyncService *m_service;
  GrpcMsgTX m_request;
  ServerAsyncResponseWriter<TxReply> m_responder;
};

class OpenChannel final : public CallData {
public:
  OpenChannel(GruutSignerService::AsyncService *service,
              ServerCompletionQueue *cq)
      : m_stream(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    m_rpc_receiver_list = RpcReceiverList::getInstance();
    proceed();
  }
  void proceed(bool st = true);

private:
  std::string m_signer_id_b64;
  RpcReceiverList *m_rpc_receiver_list;
  GruutSignerService::AsyncService *m_service;
  Identity m_request;
  ServerAsyncReaderWriter<ReplyMsg, Identity> m_stream;
};

class SignerService final : public CallData {
public:
  SignerService(GruutSignerService::AsyncService *service,
                ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    proceed();
  }
  void proceed(bool st = true);

private:
  GruutSignerService::AsyncService *m_service;
  RequestMsg m_request;
  ServerAsyncResponseWriter<MsgStatus> m_responder;
};

} // namespace gruut

#endif