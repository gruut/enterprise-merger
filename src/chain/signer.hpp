/**
 * @file signer.hpp
 * @brief Signer 구조체를 정의
 */

#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_HPP

#include <string>

namespace gruut {
using namespace std;

/**
 * @brief Signer 구조체
 */
struct Signer {
  string cert;    ///인증서
  string address; ///주소

  bool isNew() { return true; }
};
} // namespace gruut

#endif
