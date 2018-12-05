#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_HPP

#include <botan/secmem.h>
#include <string>

#include "../services/storage.hpp"
#include "types.hpp"

namespace gruut {
struct Signer {
  signer_id_type user_id{0};
  std::string pk_cert;
  hmac_key_type hmac_key;
  uint64_t last_update{0};
  SignerStatus status{SignerStatus::UNKNOWN};

  bool isNew() {
    auto cert = Storage::getInstance()->findCertificate(user_id);
    return cert.empty();
  }
};
} // namespace gruut
#endif
