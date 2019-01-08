#ifndef GRUUT_ENTERPRISE_MERGER_CRYPTO_HPP
#define GRUUT_ENTERPRISE_MERGER_CRYPTO_HPP

#include <botan-2/botan/data_src.h>
#include <botan-2/botan/pkcs8.h>
#include <regex>
#include <string>

class GemCrypto {
public:
  static bool isEncPem(std::string &&pem) { return isEncPem(pem); }

  static bool isEncPem(const std::string &pem) {
    std::regex pkcs10encsk_reg("^(-+BEGIN ENCRYPTED PRIVATE KEY-+)(.*?)(-+END "
                               "ENCRYPTED PRIVATE KEY-+)",
                               std::regex::extended);
    return std::regex_match(pem, pkcs10encsk_reg);
  }

  static bool isValidPass(std::string &&pem, std::string &&pass) {
    return isValidPass(pem, pass);
  }

  static bool isValidPass(const std::string &pem, const std::string &pass) {
    try {
      Botan::DataSource_Memory signkey_datasource(pem);
      std::unique_ptr<Botan::Private_Key> signkey(
          Botan::PKCS8::load_key(signkey_datasource, pass));
      return true;
    } catch (...) {
      return false;
    }
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_CRYPTO_HPP
