#ifndef GRUUT_ENTERPRISE_MERGER_MERGER_CLIENT_HPP
#define GRUUT_ENTERPRISE_MERGER_MERGER_CLIENT_HPP

#include "../../services/output_queue.hpp"
#include "connection_list.hpp"
#include "protos/health.grpc.pb.h"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_se.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "rpc_receiver_list.hpp"
#include <boost/asio.hpp>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
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

  void checkRpcConnection();
  void checkHttpConnection();
  void setup();

private:
  RpcReceiverList *m_rpc_receiver_list;
  ConnectionList *m_connection_list;
  Setting *m_setting;
  merger_id_type m_my_id;

  std::unique_ptr<boost::asio::deadline_timer> m_rpc_check_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_rpc_check_strand;

  std::unique_ptr<boost::asio::deadline_timer> m_http_check_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_http_check_strand;

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
  std::string getApiPath(MessageType msg_type);
};
} // namespace gruut

#endif