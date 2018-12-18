#pragma once

#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/output_queue.hpp"
#include "grpc_util.hpp"
#include <cstring>
#include <future>
#include <grpcpp/impl/codegen/status.h>
#include <iostream>
#include <string>

namespace gruut {

class MessageHandler {
public:
  MessageHandler();
  void unpackMsg(std::string &packed_msg, grpc::Status &rpc_status,
                 id_type &receiver_id);
  void packMsg(OutputMsgEntry &output_msg);

private:
  InputQueueAlt* m_input_queue;
  bool validateMessage(MessageHeader &header);
  int getMsgBodySize(MessageHeader &header);
  std::string getMsgBody(std::string &packed_msg, int body_size);
  nlohmann::json getJson(CompressionAlgorithmType compression_type,
                         std::string &body);
  std::string genPackedMsg(MessageHeader &header, nlohmann::json &body);
};

} // namespace gruut