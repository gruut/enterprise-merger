#pragma once

#include "../../utils/template_singleton.hpp"
#include "grpc_merger.hpp"
#include "protos/protobuf_signer.grpc.pb.h"
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

using namespace grpc;
using namespace grpc_signer;
namespace gruut {

enum class RpcCallStatus { CREATE, PROCESS, READ, WAIT, FINISH };

struct SignerRpcInfo {
  void *tag_identity;
  void *tag_join;
  void *tag_dhkeyex;
  void *tag_keyexfinished;
  ServerAsyncReaderWriter<GrpcMsgReqSsig, Identity> *send_req_ssig;
  ServerAsyncResponseWriter<GrpcMsgChallenge> *send_challenge;
  ServerAsyncResponseWriter<GrpcMsgResponse2> *send_response2;
  ServerAsyncResponseWriter<GrpcMsgAccept> *send_accept;
  RpcCallStatus *join_status;
  RpcCallStatus *dhkeyex_status;
  RpcCallStatus *keyexfinished_status;
};

class RpcReceiverList : public TemplateSingleton<RpcReceiverList> {
private:
  std::unordered_map<uint64_t, SignerRpcInfo> m_receiver_list;
  std::mutex m_mutex;

public:
  void
  setReqSsig(uint64_t receiver_id,
             ServerAsyncReaderWriter<GrpcMsgReqSsig, Identity> *req_sig_rpc,
             void *tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[receiver_id].send_req_ssig = req_sig_rpc;
    m_receiver_list[receiver_id].tag_identity = tag;
    m_mutex.unlock();
  }

  void setChanllenge(uint64_t receiver_id,
                     ServerAsyncResponseWriter<GrpcMsgChallenge> *challenge,
                     void *tag, RpcCallStatus *status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[receiver_id].send_challenge = challenge;
    m_receiver_list[receiver_id].tag_join = tag;
    m_receiver_list[receiver_id].join_status = status;
    m_mutex.unlock();
  }

  void setResponse2(uint64_t receiver_id,
                    ServerAsyncResponseWriter<GrpcMsgResponse2> *response2,
                    void *tag, RpcCallStatus *status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[receiver_id].send_response2 = response2;
    m_receiver_list[receiver_id].tag_dhkeyex = tag;
    m_receiver_list[receiver_id].dhkeyex_status = status;
    m_mutex.unlock();
  }

  void setAccept(uint64_t receiver_id,
                 ServerAsyncResponseWriter<GrpcMsgAccept> *accept, void *tag,
                 RpcCallStatus *status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[receiver_id].send_accept = accept;
    m_receiver_list[receiver_id].tag_keyexfinished = tag;
    m_receiver_list[receiver_id].keyexfinished_status = status;
    m_mutex.unlock();
  }

  SignerRpcInfo getSignerRpcInfo(uint64_t receiver_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SignerRpcInfo rpc_info = m_receiver_list[receiver_id];
    m_mutex.unlock();
    return rpc_info;
  }

  void clearRpcReceiverList() { m_receiver_list.clear(); }
};
} // namespace gruut