#include "message_handler.hpp"
#include "../../utils/compressor.hpp"
#include "merger_client.hpp"

namespace gruut {
void MessageHandler::unpackMsg(std::string &packed_msg,
                               grpc::Status &rpc_status,
                               uint64_t &receiver_id) {
  using namespace grpc;
  auto &input_queue = Application::app().getInputQueue();
  MessageHeader header = HeaderController::parseHeader(packed_msg);
  if (!validateMessage(header)) {
    rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Wrong Message");
    return;
  }
  int body_size = getMsgBodySize(header);
  // TODO:  HMAC 검증을 위해 key를 가져올 수 있게 되면. 가져오면 주석 해제
  //  if(header.mac_algo_type ==  MACAlgorithmType::HMAC){
  //    std::string msg = packed_msg.substr(0, HEADER_LENGTH +
  //    body_size); std::vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH
  //    + body_size , packed_msg.end()); std::vector<uint8_t> key;
  //    if(!Hmac::verifyHMAC(msg, hmac, key)){
  //      rpc_status.set_value(Status(StatusCode::UNAUTHENTICATED, "Wrong
  //      HMAC")); return;
  //    }
  //  }
  std::string msg_body = getMsgBody(packed_msg, body_size);
  nlohmann::json json_data = getJson(header.compression_algo_type, msg_body);

  if (!JsonValidator::validateSchema(json_data, header.message_type)) {
    rpc_status =
        Status(grpc::StatusCode::INVALID_ARGUMENT, "json schema check fail");
    return;
  }

  uint64_t id;
  memcpy(&id, &header.sender_id[0], sizeof(uint64_t));
  receiver_id = id;

  input_queue->emplace(make_tuple(header.message_type, id, json_data));
  rpc_status = Status::OK;
}

void MessageHandler::packMsg(OutputMessage &output_msg) {
  MessageType msg_type = get<0>(output_msg);

  nlohmann::json body = get<2>(output_msg);
  MessageHeader header;
  header.message_type = msg_type;
  // TODO : Compression type에 따라 수정 될 수 있습니다.
  header.compression_algo_type = CompressionAlgorithmType::NONE;
  std::string packed_msg = genPackedMsg(header, body);

  // TODO: hmac을 붙히는 부분, key를 가져오게되면 주석 해제
  //  if(msg_type == MessageType::MSG_ACCEPT || msg_type ==
  //  MessageType::MSG_REQ_SSIG){
  //    std::vector<uint8_t> key;
  //    std::vector<uint8_t> hmac = Hmac::generateHMAC(packed_msg, key);
  //    std::string str_hmac(hmac.begin(), hmac.end());
  //    packed_msg += str_hmac;
  //  }

  MergerClient merger_client;
  merger_client.sendMessage(msg_type, get<1>(output_msg), packed_msg);
}

bool MessageHandler::validateMessage(MessageHeader &header) {
  bool check = (header.identifier == G /*&& msg_header.version == VERSION*/);
  if (header.mac_algo_type == MACAlgorithmType::HMAC) {
    check &= (header.message_type == MessageType::MSG_SUCCESS ||
              header.message_type == MessageType::MSG_SSIG);
  }
  return check;
}

int MessageHandler::getMsgBodySize(MessageHeader &header) {
  int total_size = HeaderController::convertU8ToU32BE(header.total_length);
  int body_size = total_size - static_cast<int>(HEADER_LENGTH);
  return body_size;
}

std::string MessageHandler::getMsgBody(std::string &packed_msg, int body_size) {
  std::string packed_body = packed_msg.substr(HEADER_LENGTH, body_size);
  return packed_body;
}

nlohmann::json
MessageHandler::getJson(CompressionAlgorithmType compression_type,
                        std::string &body) {
  nlohmann::json unpacked_body;
  switch (compression_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string origin_data;
    Compressor::decompressData(body, origin_data, body.size());
    unpacked_body = nlohmann::json::parse(origin_data);
  } break;
  case CompressionAlgorithmType::NONE: {
    unpacked_body = nlohmann::json::parse(body);
  } break;
  default:
    break;
  }
  return unpacked_body;
}

std::string MessageHandler::genPackedMsg(MessageHeader &header,
                                         nlohmann::json &body) {
  std::string body_dump = body.dump();

  switch (header.compression_algo_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string compressed_body;
    Compressor::compressData(body_dump, compressed_body);
    body_dump = compressed_body;
  } break;
  case CompressionAlgorithmType ::NONE:
  default:
    break;
  }

  std::string packed_msg = HeaderController::attachHeader(
      body_dump, header.message_type, header.compression_algo_type);
  return packed_msg;
}
}; // namespace gruut