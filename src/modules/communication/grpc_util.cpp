#include "grpc_util.hpp"
#include "../../../include/json_schema.hpp"
#include "../../utils/compressor.hpp"
#include "msg_schema.hpp"
#include <botan/hex.h>
#include <botan/mac.h>
#include <cstring>

namespace gruut {
std::string
HeaderController::attachHeader(std::string &compressed_json,
                               MessageType msg_type,
                               CompressionAlgorithmType compression_algo_type) {
  std::string header;
  uint32_t total_length =
      static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

  header.resize(32);
  header[0] = G;
  header[1] = VERSION;
  header[2] = static_cast<uint8_t>(msg_type);
  if (msg_type == MessageType::MSG_ACCEPT ||
      msg_type == MessageType::MSG_REQ_SSIG) {
    header[3] = static_cast<uint8_t>(MACAlgorithmType::HMAC);
  } else {
    header[3] = static_cast<uint8_t>(MACAlgorithmType::NONE);
  }
  header[4] = static_cast<uint8_t>(compression_algo_type);
  header[5] = NOT_USED;
  for (int i = 9; i > 6; i--) {
    header[i] |= total_length;
    total_length = (total_length >> 8);
  }
  header[6] |= total_length;

  memcpy(&header[10], &LOCAL_CHAIN_ID[0], 8);
  memcpy(&header[18], &SENDER_ID[0], 8);
  memcpy(&header[26], &RESERVED[0], 6);

  return header + compressed_json;
}

std::string HeaderController::getMsgBody(std::string &raw_data, int body_size) {
  std::string json_dump = raw_data.substr(HEADER_LENGTH, body_size);

  return json_dump;
}

bool HeaderController::validateMessage(MessageHeader &msg_header) {
  // TODO: 메세지 검증할때 사용하는 값들은 변경될 수 있습니다.
  bool check = (msg_header.identifier == G && msg_header.version == VERSION);
  if (msg_header.mac_algo_type == MACAlgorithmType::HMAC) {
    check &= (msg_header.message_type == MessageType::MSG_SUCCESS ||
              msg_header.message_type == MessageType::MSG_SSIG);
  }
  return check;
}

int HeaderController::getMsgBodySize(MessageHeader &msg_header) {
  int total_size = HeaderController::convertU8ToU32BE(msg_header.total_length);
  int body_size = total_size - static_cast<int>(HEADER_LENGTH);
  return body_size;
}

// TODO: compression algorithm 추가에따라 변경될 수있습니다.
nlohmann::json
HeaderController::getJsonMessage(CompressionAlgorithmType compression_type,
                                 std::string &no_header_data) {
  nlohmann::json json_data;
  switch (compression_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string origin_data;
    Compressor::decompressData(no_header_data, origin_data,
                               no_header_data.size());
    json_data = nlohmann::json::parse(origin_data);
  } break;
  case CompressionAlgorithmType::NONE: {
    json_data = nlohmann::json::parse(no_header_data);
  } break;
  default:
    break;
  }
  return json_data;
}

MessageHeader HeaderController::parseHeader(std::string &raw_data) {
  MessageHeader msg_header;
  msg_header.identifier = static_cast<uint8_t>(raw_data[0]);
  msg_header.version = static_cast<uint8_t>(raw_data[1]);
  msg_header.message_type = static_cast<MessageType>(raw_data[2]);
  msg_header.mac_algo_type = static_cast<MACAlgorithmType>(raw_data[3]);
  msg_header.compression_algo_type =
      static_cast<CompressionAlgorithmType>(raw_data[4]);
  msg_header.dummy = static_cast<uint8_t>(raw_data[5]);
  memcpy(&msg_header.total_length[0], &raw_data[6], 4);
  memcpy(&msg_header.local_chain_id[0], &raw_data[10], 8);
  memcpy(&msg_header.sender_id[0], &raw_data[18], 8);
  memcpy(&msg_header.reserved_space[0], &raw_data[26], 6);

  return msg_header;
}

std::string HeaderController::makeHeaderAddedData(MessageHeader &msg_hdr,
                                                  nlohmann::json &json_obj) {
  std::string json_dump = json_obj.dump();
  switch (msg_hdr.compression_algo_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string compressed_json;
    Compressor::compressData(json_dump, compressed_json);
    json_dump = compressed_json;
  } break;
  case CompressionAlgorithmType::NONE:
  default:
    break;
  }
  std::string header_added_data = attachHeader(json_dump, msg_hdr.message_type,
                                               msg_hdr.compression_algo_type);

  return header_added_data;
}

grpc::Status HeaderController::analyzeData(std::string &raw_data,
                                           uint64_t &receiver_id) {
  auto &input_queue = Application::app().getInputQueue();

  MessageHeader msg_header = HeaderController::parseHeader(raw_data);
  if (!HeaderController::validateMessage(msg_header)) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Wrong Message");
  }
  int body_size = getMsgBodySize(msg_header);
  //TODO:  HMAC 검증을 위해 key를 가져올 수 있게 되면. 가져오면 주석 해제
/*if(msg_header.mac_algo_type ==  MACAlgorithmType::HMAC){
    std::string header_added_data = raw_data.substr(0, HEADER_LENGTH + body_size);
    std::vector<uint8_t> hmac(raw_data.begin() + HEADER_LENGTH + json_size , raw_data.end());
    std::vector<uint8_t> key;
    if(!Hmac::verifyHMAC(header_added_data, hmac, key))
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Wrong HMAC");
  }*/
  std::string msg_body = HeaderController::getMsgBody(raw_data, body_size);
  nlohmann::json json_data = HeaderController::getJsonMessage(
      msg_header.compression_algo_type, msg_body);

  if (!JsonValidator::validateSchema(json_data, msg_header.message_type)) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "json schema check fail");
  }
  uint64_t id;
  memcpy(&id, &msg_header.sender_id[0], sizeof(uint64_t));
  receiver_id = id;
  input_queue->emplace(
      make_tuple(msg_header.message_type, receiver_id, json_data));
  return grpc::Status::OK;
}

int HeaderController::convertU8ToU32BE(uint8_t *len_bytes) {
  return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 |
                          len_bytes[2] << 8 | len_bytes[3]);
}

bool JsonValidator::validateSchema(json json_object, MessageType msg_type) {
  using nlohmann::json;
  using nlohmann::json_schema_draft4::json_validator;

  json_validator schema_validator;
  schema_validator.set_root_schema(MessageSchema::getSchema(msg_type));

  try {
    schema_validator.validate(json_object);
    std::cout << "Validation succeeded" << std::endl;
    return true;
  } catch (const std::exception &e) {
    std::cout << "Validation failed : " << e.what() << std::endl;
    return false;
  }
}
} // namespace gruut