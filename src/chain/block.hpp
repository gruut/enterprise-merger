/**
 * @file block.hpp
 * @brief 블록과 임시블록의 구조를 정의
 */

#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "types.hpp"

namespace gruut {
/**
 * @brief 임시블록의 구조체
 */
struct PartialBlock {
  timestamp sent_time;                    ///송신 시간
  sender_id_type sender_id;               ///메시지 송신자
  chain_id_type chain_id;                 ///체인 ID
  block_height_type height;               ///블록 높이
  transaction_root_type transaction_root; ///트랜잭션의 머클루트
};

struct Block {};
} // namespace gruut
#endif
