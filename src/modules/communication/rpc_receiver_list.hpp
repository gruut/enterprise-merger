#pragma once

#include "../../utils/template_singleton.hpp"
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
  std::unordered_map<string, SignerRpcInfo> m_receiver_list;
  std::mutex m_mutex;

public:
  void
  setReqSsig(id_type &recv_id,
             ServerAsyncReaderWriter<GrpcMsgReqSsig, Identity> *req_sig_rpc,
             void *tag) {
    string recv_id_b64 = TypeConverter::toBase64Str(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[recv_id_b64].send_req_ssig = req_sig_rpc;
    m_receiver_list[recv_id_b64].tag_identity = tag;
    m_mutex.unlock();
  }

  void setChanllenge(id_type &recv_id,
                     ServerAsyncResponseWriter<GrpcMsgChallenge> *challenge,
                     void *tag, RpcCallStatus *status) {
    string recv_id_b64 = TypeConverter::toBase64Str(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[recv_id_b64].send_challenge = challenge;
    m_receiver_list[recv_id_b64].tag_join = tag;
    m_receiver_list[recv_id_b64].join_status = status;
    m_mutex.unlock();
  }

  void setResponse2(id_type &recv_id,
                    ServerAsyncResponseWriter<GrpcMsgResponse2> *response2,
                    void *tag, RpcCallStatus *status) {
    string recv_id_b64 = TypeConverter::toBase64Str(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[recv_id_b64].send_response2 = response2;
    m_receiver_list[recv_id_b64].tag_dhkeyex = tag;
    m_receiver_list[recv_id_b64].dhkeyex_status = status;
    m_mutex.unlock();
  }

  void setAccept(id_type &recv_id,
                 ServerAsyncResponseWriter<GrpcMsgAccept> *accept, void *tag,
                 RpcCallStatus *status) {
    string recv_id_b64 = TypeConverter::toBase64Str(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[recv_id_b64].send_accept = accept;
    m_receiver_list[recv_id_b64].tag_keyexfinished = tag;
    m_receiver_list[recv_id_b64].keyexfinished_status = status;
    m_mutex.unlock();
  }

  SignerRpcInfo getSignerRpcInfo(id_type &recv_id) {
    string recv_id_b64 = TypeConverter::toBase64Str(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    SignerRpcInfo rpc_info = m_receiver_list[recv_id_b64];
    m_mutex.unlock();
    return rpc_info;
  }

  void clearRpcReceiverList() { m_receiver_list.clear(); }
};
} // namespace gruut