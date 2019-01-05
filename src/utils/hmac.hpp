#ifndef GRUUT_ENTERPRISE_MERGER_HMAC_HPP
#define GRUUT_ENTERPRISE_MERGER_HMAC_HPP

#include "sha256.hpp"
#include <botan-2/botan/mac.h>
#include <string>
#include <vector>
class Hmac {
public:
  static std::vector<uint8_t> generateHMAC(std::string &msg,
                                           std::vector<uint8_t> &key) {
    std::unique_ptr<Botan::MessageAuthenticationCode> hmac =
        Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");

    // TODO : Botan 과 Bouncy castle 에서 kdf 가 맞지 않아 key가 제대로 생성되지
    // 않습니다. 따라서, Merger에서는 일반키를 이용하여 sha256후 그 키를
    // 이용하여 hmac검증과 생성을 합니다. 차 후 코드는 수정 될 예정입니다.
    std::vector<uint8_t> sha256_key = Sha256::hash(key);

    hmac->set_key(sha256_key);
    hmac->update(msg);
    return hmac->final_stdvec();
  }

  static bool verifyHMAC(std::string &msg, std::vector<uint8_t> &hmac,
                         std::vector<uint8_t> &key) {
    std::unique_ptr<Botan::MessageAuthenticationCode> mac =
        Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");
    std::vector<uint8_t> sha256_key = Sha256::hash(key);

    mac->set_key(sha256_key);
    mac->update(msg);
    return mac->verify_mac(hmac);
  }
};

#endif