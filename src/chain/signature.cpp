/**
 * @file signature.cpp
 * @brief Signer의 서명 구조체를 정의
 */

#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP

#include "types.hpp"

namespace gruut {

/**
 * @brief Signer의 서명 구조체
 */
struct Signature {
  signer_id_type signer_id;         ///Signer의 아이디
  timestamp sent_time;              ///송신 시간
  signature_type signer_signature;  ///Signer의 서명
};
} // namespace gruut
#endif
