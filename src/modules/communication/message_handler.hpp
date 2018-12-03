#pragma once

#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
#include "grpc_util.hpp"
#include <cstring>
#include <future>
#include <grpcpp/impl/codegen/status.h>
#include <iostream>
#include <string>

namespace gruut {

class MessageHandler {
public:
  void unpackMsg(std::string &packed_msg,
                 std::promise<grpc::Status> &rpc_status,
                 std::promise<uint64_t> receiver_id);
  void packMsg(OutputMessage &output_msg);

private:
  bool validateMessage(MessageHeader &header);
  int getMsgBodySize(MessageHeader &header);
  std::string getMsgBody(std::string &packed_msg, int body_size);
  nlohmann::json getJson(CompressionAlgorithmType compression_type,
                         std::string &body);
  std::string genPackedMsg(MessageHeader &header, nlohmann::json &body);
  bool checkSignerMsgType(MessageType msg_tpye);
};

} // namespace gruut