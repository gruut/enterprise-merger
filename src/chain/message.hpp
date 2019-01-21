#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_HPP

#include "nlohmann/json.hpp"
#include "types.hpp"
#include <array>
#include <tuple>

using namespace std;

namespace gruut {
constexpr int HEADER_LENGTH = 32;
constexpr int SENDER_ID_LENGTH = 8;
constexpr int RESERVED_LENGTH = 6;
constexpr int MSG_LENGTH_SIZE = 4;

constexpr uint8_t G = 'G';
constexpr uint8_t VERSION = '1';
constexpr uint8_t NOT_USED = 0x00;
constexpr array<uint8_t, RESERVED_LENGTH> RESERVED{{0x00}};

struct MessageHeader {
  uint8_t identifier;
  message_version_type version;
  MessageType message_type;
  MACAlgorithmType mac_algo_type;
  CompressionAlgorithmType compression_algo_type;
  uint8_t dummy;
  array<uint8_t, MSG_LENGTH_SIZE> total_length;
  localchain_id_type local_chain_id;
  id_type sender_id;
  array<uint8_t, RESERVED_LENGTH> reserved_space;
};
} // namespace gruut
#endif
