//
// Created by JeonilKang on 2018-11-26.
//

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
#include <string.h>

class RSA {
public:
  static std::vector<uint8_t> doSign(std::string &rsa_sk_pem, std::string &msg,
                                     bool pkcs1v15 = false) {

    std::vector<uint8_t> sig;
    std::vector<uint8_t> data(msg.begin(), msg.end());

    try {
      Botan::DataSource_Memory signkey_datasource(rsa_sk_pem);
      std::unique_ptr<Botan::Private_Key> signkey(
          Botan::PKCS8::load_key(signkey_datasource));
      Botan::RSA_PrivateKey rsa_sk(signkey->algorithm_identifier(),
                                   signkey->private_key_bits());

      sig = doSign(rsa_sk, data, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // do nothing
      std::cout << "error on PEM to RSA SK" << std::endl;
    }

    return sig;
  }

  static std::vector<uint8_t> doSign(Botan::RSA_PrivateKey &rsa_sk,
                                     std::vector<uint8_t> &data,
                                     bool pkcs1v15 = false) {
    Botan::AutoSeeded_RNG auto_rng;
    std::vector<uint8_t> sig;

    if (pkcs1v15) {
      Botan::PK_Signer signer(rsa_sk, auto_rng,
                              "EMSA3(SHA-256)"); // EMSA3 = EMSA-PKCS1-v1_5
      signer.update(data);
      sig = signer.signature(auto_rng);
    } else {
      Botan::PK_Signer signer(rsa_sk, auto_rng,
                              "EMSA4(SHA-256)"); // EMSA4 = PSS
      signer.update(data);
      sig = signer.signature(auto_rng);
    }

    return sig;
  }

  static bool doVerify(std::string &rsa_pk_pem, std::string &msg,
                       std::vector<uint8_t> &sig, bool pkcs1v15 = false) {
    std::vector<uint8_t> data(msg.begin(), msg.end());

    Botan::Public_Key *verifykey;

    bool verification_result = false;

    try {
      Botan::DataSource_Memory cert_datasource(rsa_pk_pem);
      Botan::X509_Certificate cert(cert_datasource);
      verifykey = cert.subject_public_key();

      // TODO: user certificate must be checked by the valid CA's certificate
      // TODO: We just skip this part for a while. Let's assume the certificate
      // is valid.

      Botan::RSA_PublicKey rsa_pk(verifykey->algorithm_identifier(),
                                  verifykey->public_key_bits());

      verification_result = doVerify(rsa_pk, data, sig, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // do nothing
      std::cout << "error on PEM to RSA PK" << std::endl;
    }

    delete verifykey;
    verifykey = nullptr;

    return verification_result;
  }

  static bool doVerify(Botan::RSA_PublicKey &rsa_pk, std::vector<uint8_t> &data,
                       std::vector<uint8_t> &sig, bool pkcs1v15 = false) {
    bool verification_result = false;

    if (pkcs1v15) {
      Botan::PK_Verifier verifier(rsa_pk, "EMSA3(SHA-256)");
      verification_result = verifier.verify_message(data, sig);
    } else {
      Botan::PK_Verifier verifier(rsa_pk, "EMSA4(SHA-256)");
      verification_result = verifier.verify_message(data, sig);
    }

    return verification_result;
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_SIGMANAGER_HPP
