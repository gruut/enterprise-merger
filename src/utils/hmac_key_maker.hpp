#ifndef GRUUT_ENTERPRISE_MERGER_HMAC_KEY_MAKER_HPP
#define GRUUT_ENTERPRISE_MERGER_HMAC_KEY_MAKER_HPP

#include <string>
#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/ec_group.h>
#include <botan/curve_gfp.h>
#include <botan/ecdh.h>
#include <botan/pubkey.h>

class HmacKeyMaker {
private:
  Botan::AutoSeeded_RNG m_rng;
  Botan::EC_Group m_group_domain;
  Botan::CurveGFp  m_curve;
  std::string m_kdf;

  Botan::BigInt m_secret_key;

public:
  HmacKeyMaker() : m_group_domain("secp256r1"), m_kdf("Raw") {
    m_curve = Botan::CurveGFp(m_group_domain.get_p(), m_group_domain.get_a(), m_group_domain.get_b());
  }

  ~HmacKeyMaker() = default;

  void genRandomSecretKey() {
    Botan::ECDH_PrivateKey new_rand_secret_key(m_rng, m_group_domain);
    m_secret_key = new_rand_secret_key.private_value();
  }

  void setSecretKey(std::string &my_secret_key_k_x) {
    Botan::BigInt new_secret_key(my_secret_key_k_x);
    m_secret_key = std::move(new_secret_key);
  }

  std::pair<std::string, std::string>
  getPublicKey(std::string &my_secret_key_x) {
    setSecretKey(my_secret_key_x);
    return getPublicKey();
  }

  std::pair<std::string, std::string> getPublicKey() {
    Botan::ECDH_PrivateKey my_secret_key(m_rng, m_group_domain, m_secret_key);
    Botan::PointGFp my_private_key_point = my_secret_key.public_point();
    return std::make_pair(my_private_key_point.get_x().to_hex_string(),
                          my_private_key_point.get_y().to_hex_string());
  }

  Botan::secure_vector<uint8_t>
  getSharedSecretKey(std::pair<std::string, std::string> &your_public_key,
                     int ssk_len = 32) {
    return getSharedSecretKey(your_public_key.first, your_public_key.second,
                              ssk_len);
  }

  Botan::secure_vector<uint8_t>
  getSharedSecretKey(std::string &your_public_key_x,
                     std::string &your_public_key_y, int ssk_len = 32) {

    // my_secret_key
    Botan::ECDH_PrivateKey my_secret_key(m_rng, m_group_domain, m_secret_key);

    // your_pk
    Botan::PointGFp your_public_key_point(m_curve,
                                          Botan::BigInt(your_public_key_x),
                                          Botan::BigInt(your_public_key_y));
    Botan::ECDH_PublicKey your_public_key(m_group_domain,
                                          your_public_key_point);

    // ECDH object
    Botan::PK_Key_Agreement new_ecdh(my_secret_key, m_rng, m_kdf);

    // make shared secret
    Botan::secure_vector<uint8_t> ssk =
        new_ecdh.derive_key(ssk_len, your_public_key.public_value()).bits_of();

    return ssk;
  }
};

#endif
