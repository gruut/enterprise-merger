#pragma once

#include "../../services/input_queue.hpp"
#include "protos/health.grpc.pb.h"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <atomic>
#include <grpc/support/log.h>
#include <grpcpp/ext/health_check_service_server_builder_option.h>
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
  std::unique_ptr<Server> m_server;
  std::unique_ptr<ServerCompletionQueue> m_completion_queue;
  // TODO: protobuf의 변수 명 정리후 namespace ,변수명 들은 바뀔수 있습니다.
  MergerCommunication::AsyncService m_merger_service;
  GruutSeService::AsyncService m_se_service;
  GruutNetworkService::AsyncService m_signer_service;
  RpcReceiverList *m_rpc_receivers;
  InputQueueAlt *m_input_queue;
  void recvMessage();
  std::atomic<bool> m_is_started{false};
};

class CallData {
public:
  virtual void proceed() = 0;

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
  void proceed();

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
  void proceed();

private:
  GruutSeService::AsyncService *m_service;
  GrpcMsgTX m_request;
  ServerAsyncResponseWriter<TxStatus> m_responder;
};

class OpenChannel final : public CallData {
public:
  OpenChannel(GruutNetworkService::AsyncService *service,
              ServerCompletionQueue *cq)
      : m_stream(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    m_rpc_receiver_list = RpcReceiverList::getInstance();
    proceed();
  }
  void proceed();

private:
  std::string m_signer_id_b64;
  RpcReceiverList *m_rpc_receiver_list;
  GruutNetworkService::AsyncService *m_service;
  Identity m_request;
  ServerAsyncReaderWriter<GrpcMsgReqSsig, Identity> m_stream;
};

class Join final : public CallData {
public:
  Join(GruutNetworkService::AsyncService *service, ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    m_rpc_receiver_list = RpcReceiverList::getInstance();
    proceed();
  }
  void proceed();

private:
  RpcReceiverList *m_rpc_receiver_list;
  GruutNetworkService::AsyncService *m_service;
  GrpcMsgJoin m_request;
  ServerAsyncResponseWriter<GrpcMsgChallenge> m_responder;
};

class DHKeyEx final : public CallData {
public:
  DHKeyEx(GruutNetworkService::AsyncService *service, ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    m_rpc_receiver_list = RpcReceiverList::getInstance();
    proceed();
  }
  void proceed();

private:
  RpcReceiverList *m_rpc_receiver_list;
  GruutNetworkService::AsyncService *m_service;
  GrpcMsgResponse1 m_request;
  ServerAsyncResponseWriter<GrpcMsgResponse2> m_responder;
};

class KeyExFinished final : public CallData {
public:
  KeyExFinished(GruutNetworkService::AsyncService *service,
                ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    m_rpc_receiver_list = RpcReceiverList::getInstance();
    proceed();
  }
  void proceed();

private:
  RpcReceiverList *m_rpc_receiver_list;
  GruutNetworkService::AsyncService *m_service;
  GrpcMsgSuccess m_request;
  ServerAsyncResponseWriter<GrpcMsgAccept> m_responder;
};

class SigSend final : public CallData {
public:
  SigSend(GruutNetworkService::AsyncService *service, ServerCompletionQueue *cq)
      : m_responder(&m_context) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
    proceed();
  }
  void proceed();

private:
  GruutNetworkService::AsyncService *m_service;
  GrpcMsgSsig m_request;
  ServerAsyncResponseWriter<NoReply> m_responder;
};

} // namespace gruut