#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_HPP

#include <botan/secmem.h>
#include <string>

#include "../modules/storage/storage.hpp"
#include "types.hpp"

namespace gruut {
struct Signer {
  signer_id_type user_id{0};
  std::string pk_cert;
  hmac_key_type hmac_key;
  uint64_t last_update{0};
  SignerStatus status{SignerStatus::UNKNOWN};

  // TODO: Storage에서 signer가 신규인지 아닌지 검색할 수 있는 기능 추가되면
  // 제거할 것
  bool isNew() {
    auto cert = Storage::getInstance()->findCertificate(pk_cert);
    return cert.empty();
  }
};
} // namespace gruut
#endif
