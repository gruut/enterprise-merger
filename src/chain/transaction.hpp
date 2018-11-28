/**
 * @file transaction.hpp
 * @brief 트랜잭션 구조체를 정의
 */

#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "types.hpp"

namespace gruut {
/**
 * @brief 트랜잭션 구조체
 */
struct Transaction {
  transaction_id_type transaction_id; ///트랜잭션 아이디
  timestamp sent_time;                ///송신 시간
  requestor_id_type requestor_id;     ///요청자 아이디
  TransactionType transaction_type;   ///트랜잭션 타입
  signature_type signature;           ///서명
  content_type content;               ///내용물
};

/**
 * @brief Null 트랜잭션 구조체
 */
struct NullTransaction : public Transaction {
  NullTransaction() { transaction_id = sha256(); }
};
} // namespace gruut
#endif
