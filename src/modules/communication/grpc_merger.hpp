#pragma once

#include "../../application.hpp"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

using namespace grpc;
using namespace grpc_merger;
using namespace grpc_se;
using namespace grpc_signer;

namespace gruut {

enum class CallStatus { CREATE, PROCESS, FINISH };

struct ReceiverRpcData {
  ServerReaderWriter<GrpcMsgReqSsig, Identity> *stream;
  GrpcMsgChallenge *msg_challenge;
  GrpcMsgResponse2 *msg_response2;
  GrpcMsgAccept *msg_accept;
  NoReply *no_reply;
};

class MergerRpcServer {
public:
  MergerRpcServer() : m_service_signer(*this) {}
  ~MergerRpcServer() {
    m_server_merger->Shutdown();
    m_cq_merger->Shutdown();

    m_server_signer->Shutdown();
  }
  void runMergerServ(char const *port_for_merger);
  void runSignerServ(char const *port_for_signer);

private:
  class CallData {
  public:
    virtual void proceed() = 0;
  };

  class ReceiveData final : public CallData {
  public:
    ReceiveData(MergerCommunication::AsyncService *service,
                ServerCompletionQueue *cq)
        : m_service(service), m_cq(cq), m_responder(&m_ctx),
          m_receive_status(CallStatus::CREATE) {

      proceed();
    }

    void proceed();

  private:
    MergerCommunication::AsyncService *m_service;
    ServerCompletionQueue *m_cq;

    ServerContext m_ctx;

    MergerDataRequest m_request;
    MergerDataReply m_reply;

    ServerAsyncResponseWriter<MergerDataReply> m_responder;

    CallStatus m_receive_status;
  };

  class SignerService final : public GruutNetworkService::Service {
  public:
    explicit SignerService(MergerRpcServer &server) : m_server(server) {}
    Status openChannel(ServerContext *context,
                       ServerReaderWriter<GrpcMsgReqSsig, Identity> *stream);
    Status join(ServerContext *context, const GrpcMsgJoin *msg_join,
                GrpcMsgChallenge *msg_challenge);
    // TODO: Response1 , Response2 에 대한 이름이 모호 합니다. 차후 수정 될 수
    // 있습니다.
    Status dhKeyEx(ServerContext *context,
                   const GrpcMsgResponse1 *msg_response1,
                   GrpcMsgResponse2 *msg_response2);
    Status keyExFinished(ServerContext *context,
                         const GrpcMsgSuccess *msg_success,
                         GrpcMsgAccept *msg_accept);
    Status sigSend(ServerContext *context, const GrpcMsgSsig *msg_ssig,
                   NoReply *no_reply);

  private:
    MergerRpcServer &m_server;
  };

  class SeService final : public GruutSeService::Service {
  public:
    Status transaction(ServerContext *context, const GrpcMsgTX *msg_tx,
                       Nothing *nothing);
  };

  void handleMergerRpcs();
  bool checkSignerMsgType(MessageType msg_type);
  void sendDataToSigner(std::string &header_added_data, uint64_t receiver_id,
                        MessageType msg_type);

  std::unique_ptr<ServerCompletionQueue> m_cq_merger;
  MergerCommunication::AsyncService m_service_merger;
  std::unique_ptr<Server> m_server_signer;
  std::unique_ptr<Server> m_server_merger;

  std::unordered_map<uint64_t, ReceiverRpcData> m_receiver_list;
  SignerService m_service_signer;
  SeService m_service_se;
};

class MergerRpcClient {
public:
  void run();

private:
  bool pushData(std::string &compressed_data,
                std::unique_ptr<MergerCommunication::Stub> stub);
  bool checkMergerMsgType(MessageType msg_type);
  void sendDataToMerger(std::string &header_added_data);
};
} // namespace gruut