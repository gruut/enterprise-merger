#pragma once
#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
#include <cstring>
#include <iostream>
#include <lz4.h>
#include <string>
#include <grpcpp/impl/codegen/status.h>

namespace gruut {
// TODO: 현재 테스트를 위한 고정값들 수정될 것.
constexpr uint8_t G = 'G';
constexpr uint8_t VERSION = '1';
constexpr uint8_t MAC = 0x00;
constexpr uint8_t NOT_USED = '1';
constexpr uint8_t LOCAL_CHAIN_ID[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
constexpr uint8_t SENDER_ID[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
constexpr uint8_t RESERVED[6] = {'1', '2', '3', '4', '5', '6'};

constexpr size_t HEADER_LENGTH = 32;

class HeaderController {
public:
  static std::string attachHeader(std::string &compressed_json, MessageType msg_type, MACAlgorithmType mac_algo_type, CompressionAlgorithmType compression_algo_type);
  static std::string detachHeader(std::string &raw_data, int json_size);
  static bool validateMessage(MessageHeader &msg_header);
  static int getJsonSize(MessageHeader &meg_header);
  static nlohmann::json getJsonMessage(CompressionAlgorithmType compression_type, std::string &no_header_data, int json_size);
  static MessageHeader parseHeader(std::string &raw_data);
  static std::string makeHeaderAddedData(Message &msg);
  static grpc::Status analyzeData(std::string &raw_data, uint64_t &receiver_id);
};
class JsonValidator{
public:
  static bool validateSchema(nlohmann::json json_object, MessageType msg_type);
};
} // namespace gruut