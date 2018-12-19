#pragma once

#include "../../application.hpp"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
using namespace grpc;
using namespace grpc_merger;
using namespace grpc_se;
using namespace grpc_signer;

namespace gruut {
class MergerClient {
public:
  MergerClient() { m_rpc_receiver_list = RpcReceiverList::getInstance(); }
  void sendMessage(MessageType msg_type, std::vector<id_type> &receiver_list,
                   std::vector<std::string> &packed_msg_list);

private:
  RpcReceiverList *m_rpc_receiver_list;

  void sendToSE(std::string &packed_msg);

  void sendToMerger(std::vector<id_type> &receiver_list,
                    std::string &packed_msg);
  void sendToSigner(MessageType msg_type, std::vector<id_type> &receiver_list,
                    std::vector<std::string> &packed_msg_list);

  void sendMsgToMerger(MergerInfo &merger_id, MergerDataRequest &request);

  bool checkMergerMsgType(MessageType msg_tpye);
  bool checkSignerMsgType(MessageType msg_tpye);
  bool checkSEMsgType(MessageType msg_type);
};
} // namespace gruut