#include <botan/mac.h>
#include <string>
#include <vector>
class Hmac {
public:
  static std::vector<uint8_t> generateHMAC(std::string &msg,
                                           std::vector<uint8_t> &key) {
    std::unique_ptr<Botan::MessageAuthenticationCode> hmac =
        Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");
    hmac->set_key(key);
    hmac->update(msg);
    return hmac->final_stdvec();
  }

  static bool verifyHMAC(std::string &msg, std::vector<uint8_t> &hmac,
                         std::vector<uint8_t> &key) {
    std::unique_ptr<Botan::MessageAuthenticationCode> mac =
        Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");
    mac->set_key(key);
    mac->update(msg);
    return mac->verify_mac(hmac);
  }
};