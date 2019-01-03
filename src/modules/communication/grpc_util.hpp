#pragma once
#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
#include "../../utils/hmac.hpp"
#include <cstring>
#include <grpcpp/impl/codegen/status.h>
#include <iostream>
#include <lz4.h>
#include <string>

namespace gruut {

class HeaderController {
public:
  static std::string
  attachHeader(std::string &compressed_json, MessageType msg_type,
               CompressionAlgorithmType compression_algo_type);
  static MessageHeader parseHeader(std::string &raw_data);
  static int convertU8ToU32BE(uint8_t *len_bytes);
};
class JsonValidator {
public:
  static bool validateSchema(json json_object, MessageType msg_type);
};
} // namespace gruut
