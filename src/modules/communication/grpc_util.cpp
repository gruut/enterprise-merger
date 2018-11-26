#include "grpc_util.hpp"
#include "../../../include/json_schema.hpp"
#include "../../utils/compressor.hpp"
#include "msg_schema.hpp"
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

std::string HeaderController::detachHeader(std::string &raw_data) {
  std::string json_dump(raw_data.begin() + HEADER_LENGTH, raw_data.end());

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

int HeaderController::getJsonSize(MessageHeader &msg_header) {
  int json_size = 0;
  for (int i = 3; i > 0; i--) {
    json_size |= msg_header.total_length[i];
    json_size = (json_size << 8);
  }
  json_size |= msg_header.total_length[0];
  json_size -= HEADER_LENGTH;
  return json_size;
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
  msg_header.identifier = G;
  msg_header.version = VERSION;
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
  std::string no_header_data = HeaderController::detachHeader(raw_data);

  nlohmann::json json_data = HeaderController::getJsonMessage(
      msg_header.compression_algo_type, no_header_data);

  if (!JsonValidator::validateSchema(json_data, msg_header.message_type)) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "json schema check fail");
  }
  uint64_t id;
  memcpy(&id, &msg_header.sender_id[0], sizeof(uint64_t));
  receiver_id = id;
  input_queue->push(make_tuple(msg_header.message_type, json_data));
  return grpc::Status::OK;
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