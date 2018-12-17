#ifndef GRUUT_ENTERPRISE_MERGER_SIGMANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGMANAGER_HPP

#include <botan/auto_rng.h>
#include <botan/data_src.h>
#include <botan/exceptn.h>
#include <botan/pkcs8.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/x509cert.h>
#include <iostream>
#include <string>
#include <vector>

class RSA {
public:
  static std::vector<uint8_t> doSign(std::string &rsa_sk_pem, std::string &msg,
                                     bool pkcs1v15 = false,
                                     const std::string &pass = "") {
    try {
      auto rsa_sk = getPrivateKey(rsa_sk_pem, pass);
      std::vector<uint8_t> data(msg.begin(), msg.end());
      std::vector<uint8_t> sig = doSign(rsa_sk, data, pkcs1v15);

      return sig;
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to RSA SK: " << exception.what() << std::endl;

      return std::vector<uint8_t>();
    }
  }

  static std::vector<uint8_t> doSign(std::string &rsa_sk_pem,
                                     const std::vector<uint8_t> &data,
                                     bool pkcs1v15 = false,
                                     const std::string &pass = "") {
    try {
      auto rsa_sk = getPrivateKey(rsa_sk_pem, pass);
      return doSign(rsa_sk, data, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // TODO: logging
      std::cout << "error on PEM to RSA SK: " << exception.what() << std::endl;

      return std::vector<uint8_t>();
    }
  }

  static std::vector<uint8_t> doSign(Botan::RSA_PrivateKey &rsa_sk,
                                     const std::vector<uint8_t> &data,
                                     bool pkcs1v15 = false) {
    Botan::AutoSeeded_RNG auto_rng;
    Botan::PK_Signer signer(rsa_sk, auto_rng,
                            getEmsa(pkcs1v15)); // EMSA3 = EMSA-PKCS1-v1_5

    return signer.sign_message(data, auto_rng);
  }

  static bool doVerify(const std::string &rsa_pk_pem, const std::string &msg,
                       const std::vector<uint8_t> &sig, bool pkcs1v15 = false) {
    try {
      Botan::DataSource_Memory cert_datasource(rsa_pk_pem);
      Botan::X509_Certificate cert(cert_datasource);
      Botan::RSA_PublicKey public_key(cert.subject_public_key_algo(),
                                      cert.subject_public_key_bitstring());

      // TODO: user certificate must be checked by the valid CA's certificate
      // TODO: We just skip this part for a while. Let's assume the certificate
      // is valid.

      const std::vector<uint8_t> data(msg.begin(), msg.end());
      return doVerify(public_key, data, sig, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to RSA PK: " << exception.what() << std::endl;
      return false;
    }
  }

  static bool doVerify(Botan::Public_Key &rsa_pk,
                       const std::vector<uint8_t> &data,
                       const std::vector<uint8_t> &sig, bool pkcs1v15 = false) {
    Botan::PK_Verifier verifier(rsa_pk, getEmsa(pkcs1v15));
    return verifier.verify_message(data, sig);
  }

  static bool doVerify(const std::string &rsa_pk,
                       const std::vector<uint8_t> &data,
                       const std::vector<uint8_t> &sig, bool pkcs1v15 = false) {
    try {
      Botan::DataSource_Memory cert_datasource(rsa_pk);
      Botan::X509_Certificate cert(cert_datasource);
      Botan::RSA_PublicKey public_key(cert.subject_public_key_algo(),
                                      cert.subject_public_key_bitstring());

      return doVerify(public_key, data, sig, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to RSA PK: " << exception.what() << std::endl;
      return false;
    }
  }

private:
  static std::string getEmsa(bool is_pkcs1v15) {
    return is_pkcs1v15 ? "EMSA3(SHA-256)" : "EMSA4(SHA-256)";
  }

  static Botan::RSA_PrivateKey getPrivateKey(std::string &rsa_sk_pem,
                                             const std::string &pass = "") {

    try {
      Botan::DataSource_Memory signkey_datasource(rsa_sk_pem);
      std::unique_ptr<Botan::Private_Key> signkey(
          Botan::PKCS8::load_key(signkey_datasource, pass));
      return Botan::RSA_PrivateKey(signkey->algorithm_identifier(),
                                   signkey->private_key_bits());
    } catch (Botan::Exception &exception) {
      throw;
    }
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_SIGMANAGER_HPP
