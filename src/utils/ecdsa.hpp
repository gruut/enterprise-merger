#pragma once

#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/emsa1.h>
#include <botan-2/botan/pk_keys.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/x509cert.h>
#include <iostream>
#include <vector>

namespace gruut {

class ECDSA {
public:
  static std::vector<uint8_t> doSign(std::string &pem, std::string &msg) {
    try {
      auto ecdsa_sk = getPrivateKey(pem);
      return doSign(msg, ecdsa_sk);
    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static std::vector<uint8_t> doSign(std::string &msg,
                                     Botan::ECDSA_PrivateKey &sk) {
    try {
      std::vector<uint8_t> data(msg.data(), msg.data() + msg.length());
      Botan::AutoSeeded_RNG rng;
      Botan::ECDSA_PrivateKey sign_key(sk->algorithm_identifier(),
                                       sk->private_key_bits());
      Botan::PK_Signer signer(sign_key, rng, "EMSA1(SHA-256)");
      signer.update(data);
      std::vector<uint8_t> signature = signer.signature(rng);
      return signature;
    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool doVerify(std::string &ca_cert_pem, std::string &cert_pem,
                       std::string &text, std::vector<uint8_t> &sig) {
    try {
      Botan::DataSource_Memory ca_cert_datasource(ca_cert_pem);
      Botan::X509_Certificate ca_cert(ca_cert_datasource);
      // ca_cert
      Botan::DataSource_Memory cert_datasource(cert_pem);
      Botan::X509_Certificate cert(cert_datasource);
      // cert
      Botan::ECDSA_Publickey verify_key(cert.subject_public_key());
      return doVerify(text, sig, verify_key)
    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool doVerify(std::string &text, std::vector<uint8_t> &sig,
                       Botan::ECDSA_Publickey &pk) {
    try {
      Botan::PK_Verifier verifier(pk, "EMSA1(SHA-256)");
      return verifier.verify_message((const uint8_t *)text.data(), text.size(),
                                     (const uint8_t *)sig.data(), sig.size())
    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  //  bool setCACertPem(std::string &ca_cert_pem) {
  //
  //    Botan::X509_Certificate ca_cert = getCertFromPemStr(ca_cert_pem);
  //
  //    if(!ca_cert.is_CA_cert()){
  //      return false;
  //    }
  //
  //    Botan::ECDSA_PublicKey ca_pub_key =
  //    getPubKeyFromPtr(ca_cert.subject_public_key());
  //
  //
  //    if(!ca_cert.check_signature(ca_pub_key)) {
  //      return false;
  //    }
  //
  //    if(!isValidCert(ca_cert)) {
  //      return false;
  //    }
  //
  //    delete m_ca_pub_key;
  //    m_ca_pub_key = ca_cert.subject_public_key();
  //
  //    return true;
  //  }

  //  bool setCertPem(std::string &cert_pem) {
  //
  //    Botan::X509_Certificate cert = getCertFromPemStr(cert_pem);
  //    Botan::ECDSA_PublicKey ca_pub_key = getPubKeyFromPtr(m_ca_pub_key);
  //
  //    if(!cert.check_signature(ca_pub_key)) {
  //      return false;
  //    }
  //
  //    if(!isValidCert(cert)) {
  //      return false;
  //    }
  //
  //    delete m_verify_key;
  //    m_verify_key = cert.subject_public_key();
  //
  //    return true;
  //  }

private:
  static Botan::ECDSA_PrivateKey getPrivateKey(std::string &ecdsa_sk_pem) {
    ecdsa_sk_pem = R"USK(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDfkl/sCbp4F3LO
DlyZgev7+0u+AqHysgrQL2QkEVnxr+x7tewNaNoXUXFubhgIKtjulgCCUHM60ldq
3WaLpM2AT1QZ3K+j75HiwTYlJ8ISsIsS+3E9VXR/T0uM6bZnjpRzcBVdJ1NUJqw4
nEzd11rbT8uVlPTGbsG0GljsiylT9lyo3h3a9o1tpnIIhC3NlXjTd4zqX3YCGwBy
6w7CU0t3D7un9GXONiz328rLEuHP2ZE++xueMki6uRFKNhf4LiOM0O8Qkw4IIIdQ
zbHLx3JgWkkWqqBx2OTp+tYEwZthFO9fgs0bhvZA7L9IBy9M6VCL9spEsz7ddAaE
ZcFcyfvlAgMBAAECggEBAM+JzJuLiFrUwZEAiftCPPMkGvKe9QEbP6h0ZcyJguo1
uhw5C5CDJfkBdH/jmVFznP8VphFSZzVSby3Xqsq0yMN0YIjFcRKIYO+TFhU1rBW3
ZtLPMRaTjlpkHKkJh3boR2xFvr9Dsznp0HOYvE4vDLuLflwz82mFBTGQR74FjO7P
hoR4LIQ+Pk5MuyI7Q7Jn0DZ7hkNrnL1g5g+UCxGy/CDNPlGw48t9Ftpsy+z0gVan
Cgl3fAW0+ufHvm3QyxMlNmK2zuQKNcJSLeTTEtURV8ww5ZuKB9XEMOm2GzMCakRd
eeb1/wPqwyQEMHT9Ll01R2dIjU8u1ICI1x/R7rhL6mECgYEA86+LZnwGS18gY5Ck
6cqCvo38cA9F70CrU4FK/J3BboXME59i5fcUTSUA7l20cXi+hS2fAp81sKUc3nAu
tp0/B6dVeuzgVhfboRatM6BhGYHu+BjfwHAjPbgM7VBzJpVAwguuCO4ia8MWZZAk
Rd78QSbtKtgY+d8bZjxe6Kh/cz0CgYEA6t6f91fBq1mx0UnunduP+rTs/4fhGeIJ
b94P37C/36lkAsucYzZEOPV1YEWxw4OlN40BIco125KhWj6KE3HqrhGeaB0cuwqz
GuPFbO3MdDj51SUYrcGJuj2edXKZyCgsd6wFp5FWSdH8n8amQbk3aHmV1GUsys0V
BKWf0P+elckCgYAuuGhcpMi8KKfYDwJfRJFeoXBVt8frwBVY9EABQOm2G/btiDB4
8K82vzJ3gQW4f7Lfa8jBwu6TSITJbO632lwcRovP/pxgRUC5mNRqQoR7VHsRnAtC
JP3Mtn3b/gGl0xXQXlbmpWl6CbRAkqsxrjfk8eakwTvApHLnXgnAR5Xv7QKBgQCC
kUqahVWr/UwGDjSx2wp6lDQghhhUfD1EzE1EzIyOOSvZBfoliVh51bLv1y7QgxHJ
BQE5GKHCNAyxD41Q7AZLyI2oUW7UaElTTIZHXRdJEReKL3o9thbryy+ZGSF2jSbT
THVER16R4UOwSw3IAcBUuyrZDXnOMB5cG/rxg/lUSQKBgEtqijnRFo/CM4QpjXgH
okpmJkjwucI3JAGcdZ+tNgUxwKnmwhgkbS7NVuLkZlNqVHfRMzbzDIxprPFy9t5T
wCHB5M/bx9u2PbYfUTJq0m+qJyuWOCxbqi4JNRIKk9Lvy5EBBneU4wrCRovlh7Jg
Wcql+Gep9ebzfGArFp7anHE9
-----END PRIVATE KEY-----)USK";

    Botan::DataSource_Memory signkey_datasource(ecdsa_sk_pem);
    std::unique_ptr<Botan::Private_Key> signkey(
        Botan::PKCS8::load_key(signkey_datasource));
    return Botan::ECDSA_privateKey(signkey->algorithm_identifier(),
                                   signkey->public_key_bits());
  }
};
} // namespace gruut