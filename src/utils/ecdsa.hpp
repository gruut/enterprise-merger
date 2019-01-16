#ifndef GRUUT_ENTERPRISE_MERGER_ECDSA_HPP
#define GRUUT_ENTERPRISE_MERGER_ECDSA_HPP

#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/emsa1.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/pk_keys.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/x509cert.h>
#include <iostream>
#include <string>
#include <vector>

class ECDSA {
public:
  static std::vector<uint8_t> doSign(std::string &ecdsa_sk_pem,
                                     std::string &msg,
                                     const std::string &pass = "") {
    try {
      auto ecdsa_sk = getPrivateKey(ecdsa_sk_pem, pass);
      std::vector<uint8_t> data(msg.begin(), msg.end());
      std::vector<uint8_t> sig = doSign(ecdsa_sk, data);

      return sig;
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to ECDSA SK: " << exception.what()
                << std::endl;

      return std::vector<uint8_t>();
    }
  }

  static std::vector<uint8_t> doSign(std::string &ecdsa_sk_pem,
                                     const std::vector<uint8_t> &data,
                                     const std::string &pass = "") {
    try {
      auto ecdsa_sk = getPrivateKey(ecdsa_sk_pem, pass);

      return doSign(ecdsa_sk, data);
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to ECDSA SK: " << exception.what()
                << std::endl;

      return std::vector<uint8_t>();
    }
  }

  static std::vector<uint8_t> doSign(Botan::ECDSA_PrivateKey &ecdsa_sk,
                                     const std::vector<uint8_t> &data) {
    Botan::AutoSeeded_RNG auto_rng;
    Botan::PK_Signer signer(ecdsa_sk, auto_rng, "EMSA1(SHA-256)",
                            Botan::Signature_Format::DER_SEQUENCE);
    return signer.sign_message(data, auto_rng);
  }

  static bool doVerify(const std::string &ecdsa_pk_pem, const std::string &msg,
                       const std::vector<uint8_t> &sig) {
    try {
      Botan::DataSource_Memory cert_datasource(ecdsa_pk_pem);
      Botan::X509_Certificate cert(cert_datasource);
      Botan::ECDSA_PublicKey public_key(cert.subject_public_key_algo(),
                                        cert.subject_public_key_bitstring());

      const std::vector<uint8_t> data(msg.begin(), msg.end());
      return doVerify(public_key, data, sig);
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to ECDSA PK: " << exception.what()
                << std::endl;
      return false;
    }
  }

  static bool doVerify(Botan::Public_Key &ecdsa_pk,
                       const std::vector<uint8_t> &data,
                       const std::vector<uint8_t> &sig) {
    Botan::PK_Verifier verifier(ecdsa_pk, "EMSA1(SHA-256)",
                                Botan::Signature_Format::DER_SEQUENCE);
    return verifier.verify_message(data, sig);
  }

  static bool doVerify(const std::string &ecdsa_pk,
                       const std::vector<uint8_t> &data,
                       const std::vector<uint8_t> &sig) {
    try {
      Botan::DataSource_Memory cert_datasource(ecdsa_pk);
      Botan::X509_Certificate cert(cert_datasource);
      Botan::ECDSA_PublicKey public_key(cert.subject_public_key_algo(),
                                        cert.subject_public_key_bitstring());

      return doVerify(public_key, data, sig);
    } catch (Botan::Exception &exception) {
      // TODO: Logging
      std::cout << "error on PEM to ECDSA PK: " << exception.what()
                << std::endl;
      return false;
    }
  }

private:
  static Botan::ECDSA_PrivateKey getPrivateKey(std::string &ecdsa_sk_pem,
                                               const std::string &pass = "") {
    try {
      Botan::DataSource_Memory signkey_datasource(ecdsa_sk_pem);
      std::unique_ptr<Botan::Private_Key> signkey(
          Botan::PKCS8::load_key(signkey_datasource, pass));

      return Botan::ECDSA_PrivateKey(signkey->algorithm_identifier(),
                                     signkey->private_key_bits());
    } catch (Botan::Exception &exception) {
      throw;
    }
  }
};

#endif