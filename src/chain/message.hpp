/**
 * @file message.hpp
 * @brief 메시지와 메시지 헤더의 구조체를 정의
 */

#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_HPP

#include "../../include/nlohmann/json.hpp"
#include "types.hpp"
#include <tuple>

using namespace std;

namespace gruut {

/**
 * @brief 메시지 헤더 구조체
 */
struct MessageHeader {
  uint8_t identifier;                                     ///식별자(G)
  uint8_t version;                                        ///버전(메인, 서브)
  MessageType message_type;                               ///메시지 타입
  MACAlgorithmType mac_algo_type = MACAlgorithmType::RSA; ///MAC 타입
  CompressionAlgorithmType compression_algo_type;         ///압축 타입
  uint8_t dummy;                                          ///미사용
  uint8_t total_length[4];                                ///총 길이
  local_chain_id_type local_chain_id[8];                  ///로컬체인 아이디
  uint8_t sender_id[8];                                   ///송신자
  uint8_t reserved_space[6];                              ///예비 할당
};

using InputMessage = tuple<MessageType, uint64_t, nlohmann::json>;
using OutputMessage = tuple<MessageType, vector<uint64_t>, nlohmann::json>;

/**
 * @brief 메시지 구조체
 */
struct Message : public MessageHeader {
  Message() = delete;

  Message(MessageHeader &header) : MessageHeader(header) {}

  nlohmann::json data;
};
} // namespace gruut
#endif
