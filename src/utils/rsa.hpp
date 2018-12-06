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

class RSA {
public:
  static std::vector<uint8_t> doSign(std::string &rsa_sk_pem, std::string &msg,
                                     bool pkcs1v15 = false) {
    try {
      auto rsa_sk = getPrivateKey(rsa_sk_pem);
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
                                     std::vector<uint8_t> &data,
                                     bool pkcs1v15 = false) {
    try {
      auto rsa_sk = getPrivateKey(rsa_sk_pem);
      return doSign(rsa_sk, data, pkcs1v15);
    } catch (Botan::Exception &exception) {
      // TODO: logging
      std::cout << "error on PEM to RSA SK: " << exception.what() << std::endl;

      return std::vector<uint8_t>();
    }
  }

  static std::vector<uint8_t> doSign(Botan::RSA_PrivateKey &rsa_sk,
                                     std::vector<uint8_t> &data,
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

  static bool doVerify(std::string &rsa_pk, const std::vector<uint8_t> &data,
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

  static Botan::RSA_PrivateKey getPrivateKey(std::string &rsa_sk_pem) {
    if (rsa_sk_pem.empty()) {
      rsa_sk_pem =
          "-----BEGIN PRIVATE KEY-----\n"
          "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCKWX3bNHseiGPQ\n"
          "MSzoJt0kGmPlhp7BYj2LLPEexBl2RNFPyqhpmgROlY91GbQTUB9B5/wR+agn/bMF\n"
          "6jKtNH27HqWXeiJxtlDCknOjJZLbdhhwynyWxmzgHDG34beuHK8rYQYzcXuOcGPK\n"
          "KP0impIzs8jQZQfJu64bU9GjY7ElVvQNBzOHODBpCzpv6AQ2UXfXt57T/vNAG6UM\n"
          "fuB+uTrW8q4d3raHsy7VPEUG3os9wteny5OZdIQMaeSixtUMJjH0BeaTaEg3Guzx\n"
          "LV/YkZzCJ7HOXmU1DlXWCk/L0/w1sseKwIohS3WPXzsdTbU3zfQzvHKgCR3wkkUT\n"
          "S5csmTxpAgMBAAECggEAYiFp8NK/xX9udNx8gsoWLyZ81u/uqTJaft5IxM7JVKcp\n"
          "ZBRV8llpVqgk0iWCIfTBxwiaNdHEYWFE2xwsB8jkqZXqVJAv4EI19FzWotDi4sFY\n"
          "QqCNUJC75xZ4eXojw97arMUsFc8XmYfEcD80lZfXvc520MHojUGPFBkW6HKs0tNY\n"
          "S5oJrSg/o05j6jTlDmHlm+aswUGfL2SwkEgz80mahQxsHe4/RPfyxPF84BMXX3+8\n"
          "8Z2FaK0qVIeXFrso96dAjPFKcrreXRn7lUze7Rw0i5Vy/8hKAzezQ5nstwn/3s78\n"
          "lO4QuGuyingNwLO4D9P4L/rP4GHgJ3DKFLFOC1YEAQKBgQDAYQO+yFWcwBV+dDhf\n"
          "zyXSk8IV5hPi7Wa1IVFuSQMmCN8dFSxwdsjGli2q6LFYckyVSZyy6E2k7c0xwNs/\n"
          "A2xUTX5xUby2l3S3rn3Mx2DqbjDHv1zGqYfgAZReBADUnSy3hwnMz2a2jTXp7fOA\n"
          "T3+1jvf8SEqwtife9MEMxfjAgQKBgQC4GkxDuxzyv7tYpE+HqN7/P3mIPzI68sdD\n"
          "5btzyaQvICZcjer8OZiF1juDi+VPo1NA22Qki2cUVNSZNNWnGg5Le+06YovQZ8Qp\n"
          "xsn2qJpohbSEUAy6dqea7Kh58fNbAZFiE7mdvqWAj9oGH7LNMZg7s/08fpqLbBLt\n"
          "CVCB+K6H6QKBgQC+/NGCC1NrPgtYsjrxay6qgwSRRwyBIpzvv7cfHR8iGHagYc/v\n"
          "iw3CkX+fCEpge4DqSN1nhFbpISiwdz1yroxSmWipSbNnNq+qV3IO5fWiZ2jINYP+\n"
          "unnpesf4GlNUwQGO5mJlUZYwL7rRlelDfilUby5k6MQ18XFd2HD7pGNTgQKBgD4I\n"
          "Dl5b85sPY06wvmNVUR3sA0UXFhOqrd2A0LJo5LtEN+jDoMOvnGasEo12W6ODwo99\n"
          "3LY7ilXdZ2zf0oVlUB+69+nOPpHQBNaWtoI3uR8yveo/FqrVRA/9YZ8FGRw24QeM\n"
          "4eP20skIr0uU7qgY59RmBxOVDPmhRpc7pjbE1fnRAoGAXjtXWSLZbsYWyuynLfuh\n"
          "w5irsKW+Bc/8puCL+iQK1arhGF1rNIYiAXNgSfflLMCcw3usIuSRl+yQKRf++Y3s\n"
          "tJ5nrk1ubr+bdyW9edu1hKIaRq5s0jXNojvh//F/FRuKeVCW/PiUiRUBSzJhQFim\n"
          "CQFeieBTXhpq0LgoxS9r5O4=\n"
          "-----END PRIVATE KEY-----";
    }

    Botan::DataSource_Memory signkey_datasource(rsa_sk_pem);
    std::unique_ptr<Botan::Private_Key> signkey(
        Botan::PKCS8::load_key(signkey_datasource));
    return Botan::RSA_PrivateKey(signkey->algorithm_identifier(),
                                 signkey->private_key_bits());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_SIGMANAGER_HPP
