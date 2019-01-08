#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_HPP

#include <botan-2/botan/secmem.h>
#include <string>

#include "../services/storage.hpp"
#include "types.hpp"

namespace gruut {
struct Signer {
  signer_id_type user_id;
  std::string pk_cert;
  hmac_key_type hmac_key;
  timestamp_type last_update{0};
  SignerStatus status{SignerStatus::UNKNOWN};

  bool isNew() {
    auto cert = Storage::getInstance()->getCertificate(user_id);
    return (cert.empty() || cert != pk_cert);
  }
};
} // namespace gruut
#endif
