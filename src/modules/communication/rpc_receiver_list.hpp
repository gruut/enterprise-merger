#ifndef GRUUT_ENTERPRISE_MERGER_RPC_RECEIVER_LIST_HPP
#define GRUUT_ENTERPRISE_MERGER_RPC_RECEIVER_LIST_HPP

#include "../../utils/template_singleton.hpp"
#include "../../utils/type_converter.hpp"
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
  ServerAsyncReaderWriter<ReplyMsg, Identity> *send_msg;
};

class RpcReceiverList : public TemplateSingleton<RpcReceiverList> {
private:
  std::unordered_map<string, SignerRpcInfo> m_receiver_list;
  std::mutex m_mutex;

public:
  void setReplyMsg(std::string &recv_id_b64,
                   ServerAsyncReaderWriter<ReplyMsg, Identity> *reply_rpc,
                   void *tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list[recv_id_b64].send_msg = reply_rpc;
    m_receiver_list[recv_id_b64].tag_identity = tag;
    m_mutex.unlock();
  }

  void eraseRpcInfo(std::string &recv_id_b64) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiver_list.erase(recv_id_b64);
    m_mutex.unlock();
  }

  SignerRpcInfo getSignerRpcInfo(id_type &recv_id) {
    string recv_id_b64 = TypeConverter::encodeBase64(recv_id);
    std::lock_guard<std::mutex> lock(m_mutex);
    SignerRpcInfo rpc_info = m_receiver_list[recv_id_b64];
    m_mutex.unlock();
    return rpc_info;
  }

  void clearRpcReceiverList() { m_receiver_list.clear(); }
};
} // namespace gruut

#endif