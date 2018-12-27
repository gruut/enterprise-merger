#pragma once

#include "../../services/output_queue.hpp"
#include "connection_list.hpp"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <boost/asio.hpp>
#include <grpc/support/log.h>
#include <grpcpp/ext/health_check_service_server_builder_option.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>
#include <memory>
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

  void checkConnection();
  void setup();

private:
  RpcReceiverList *m_rpc_receiver_list;
  ConnectionList *m_connection_list;
  Setting *m_setting;
  merger_id_type m_my_id;

  std::unique_ptr<boost::asio::deadline_timer> m_conn_check_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_conn_check_strand;

  void sendToSE(std::vector<id_type> &receiver_list,
                OutputMsgEntry &output_msg);

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