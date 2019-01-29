#ifndef GRUUT_ENTERPRISE_MERGER_MERGER_CLIENT_HPP
#define GRUUT_ENTERPRISE_MERGER_MERGER_CLIENT_HPP

#include "../../services/output_queue.hpp"
#include "../../utils/periodic_task.hpp"
#include "connection_list.hpp"
#include "manage_connection.hpp"
#include "protos/health.grpc.pb.h"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

using namespace grpc;
using namespace grpc_merger;
using namespace grpc_se;
using namespace grpc_signer;

namespace gruut {
class MergerClient {
public:
  MergerClient();
  void sendMessage(MessageType msg_type, std::vector<id_type> &receiver_list,
                   std::vector<std::string> &packed_msg_list,
                   OutputMsgEntry &output_msg);

  void accessToTracker();
  void checkConnection();
  void setup();

private:
  RpcReceiverList *m_rpc_receiver_list;
  ConnManager *m_conn_manager;
  bool m_disable_tracker{false};

  PeriodicTask m_rpc_check_scheduler;
  PeriodicTask m_http_check_scheduler;

  void checkRpcConnection();
  void checkHttpConnection();
  json sendToTracker(OutputMsgEntry &output_msg);

  void sendToSE(std::vector<id_type> &receiver_list, OutputMsgEntry &output_msg,
                std::string api_path);

  void sendToMerger(std::vector<id_type> &receiver_list,
                    std::string &packed_msg);
  void sendToSigner(MessageType msg_type, std::vector<id_type> &receiver_list,
                    std::vector<std::string> &packed_msg_list);

  void sendMsgToMerger(MergerInfo &merger_id, MergerDataRequest &request);

  bool checkMergerMsgType(MessageType msg_tpye);
  bool checkSignerMsgType(MessageType msg_tpye);
  bool checkSEMsgType(MessageType msg_type);
  bool checkTrackerMsgType(MessageType msg_type);
  Status sendHealthCheck(MergerInfo &merger_info);
  std::string getApiPath(MessageType msg_type);
};
} // namespace gruut

#endif