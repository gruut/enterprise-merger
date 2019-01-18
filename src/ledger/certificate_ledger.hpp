#ifndef GRUUT_ENTERPRISE_MERGER_CERTIFICATE_HPP
#define GRUUT_ENTERPRISE_MERGER_CERTIFICATE_HPP

#include "../services/storage.hpp"
#include "../utils/safe.hpp"
#include "ledger.hpp"

#include <botan-2/botan/asn1_alt_name.h>
#include <botan-2/botan/asn1_time.h>
#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/calendar.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/hex.h>
#include <botan-2/botan/pk_keys.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509_ext.h>
#include <botan-2/botan/x509_obj.h>
#include <botan-2/botan/x509cert.h>

#include <chrono>
#include <iostream>
#include <vector>

namespace gruut {

class CertificateLedger : public Ledger {
public:
  CertificateLedger() { setPrefix("C"); }

  bool isValidTx(const Transaction &tx) override { return true; }

  bool procBlock(const json &block_json) override {
    if (!block_json["tx"].is_array())
      return false;

    for (auto &tx_json : block_json["tx"]) {
      if (isValidTxInBlock(tx_json)) {
        saveCert(tx_json);
      }
    }

    m_storage->flushLedger();

    return true;
  }

  std::string getCertificate(const std::string &user_id_b64,
                             const timestamp_t &at_this_time = 0) {
    std::string cert;
    std::string cert_size = searchLedger(user_id_b64);

    if (!cert_size.empty()) {
      int num_certs = stoi(cert_size);
      if (at_this_time == 0) {
        std::string temp = user_id_b64 + "_" + to_string(num_certs - 1);
        std::string latest_cert = searchLedger(temp);

        json latest_cert_json = Safe::parseJsonAsArray(latest_cert);
        if (!latest_cert_json.empty())
          cert = Safe::getString(latest_cert_json[2]);
      } else {

        timestamp_t max_start_date = 0;
        for (int i = 0; i < num_certs; ++i) {
          std::string temp = user_id_b64 + "_" + to_string(i);
          std::string nth_cert = searchLedger(temp);

          json cert_json = Safe::parseJson(nth_cert);
          if (cert_json.empty())
            break;

          auto start_date =
              static_cast<timestamp_t>(stoi(Safe::getString(cert_json[0])));
          auto end_date =
              static_cast<timestamp_t>(stoi(Safe::getString(cert_json[1])));
          if (start_date <= at_this_time && at_this_time <= end_date) {
            if (max_start_date < start_date) {
              max_start_date = start_date;
              cert = Safe::getString(cert_json[2]);
            }
          }
        }
      }
    }

    return cert;
  }

  std::string getCertificate(const signer_id_type &user_id,
                             const timestamp_t &at_this_time = 0) {
    std::string user_id_b64 = TypeConverter::encodeBase64(user_id);
    return getCertificate(user_id_b64);
  }

private:
  bool isValidCaCert(const std::string &ca_cert_pem) {
    try {
      Botan::X509_Certificate ca_cert = strToCert(ca_cert_pem);

      if (!ca_cert.is_CA_cert()) {
        std::cout << "X509_CA: This certificate is not for a CA" << std::endl;
        return false;
      }

      Botan::RSA_PublicKey ca_pub_key(
          ca_cert.subject_public_key()->algorithm_identifier(),
          ca_cert.subject_public_key()->public_key_bits());

      if (!ca_cert.check_signature(ca_pub_key)) {
        return false;
      }
      return isValidCaCert(ca_cert);

    } catch (Botan::Exception &exception) {
      std::cout << "error on PEM to RSA PK: " << exception.what() << std::endl;
    }
    return true;
  }

  bool isValidCaCert(const Botan::X509_Certificate &ca_cert) {

    // TODO : cert 정보는 하드코딩 한 상태 -> 나중에 수정
    /*
    if (validTime(ca_cert) != true)
      return false;
    if (ca_cert.subject_info("Name")[0] != "//////////8=")
      return false;
    if (ca_cert.subject_info("RFC822")[0] != "contact@gruut.net")
      return false;
    if (ca_cert.subject_info("Organization")[0] != "Gruut Networks")
      return false;
    if (ca_cert.subject_info("Country")[0] != "KR")
      return false;
    if (ca_cert.issuer_info("Name")[0] != "//////////8=")
      return false;
    if (ca_cert.issuer_info("Organization")[0] != "Gruut Networks")
      return false;
    if (Botan::hex_encode(ca_cert.serial_number()) !=
        "BD9171B2F8905C20AB9CA974143C031C")
      return false;
    */
    return true;
  }

  bool isValidCert(const std::string &ca_cert_pem, const std::string &cert_pem_str) {
    try {
      Botan::X509_Certificate ca_cert = strToCert(ca_cert_pem);
      Botan::X509_Certificate cert = strToCert(cert_pem_str);

      Botan::RSA_PublicKey ca_pub_key(
          ca_cert.subject_public_key()->algorithm_identifier(),
          ca_cert.subject_public_key()->public_key_bits());

      if (!cert.check_signature(ca_pub_key)) {
        return false;
      }

      return isValidCert(cert);

    } catch (Botan::Exception &exception) {
      std::cout << "error on PEM to ECDSA PK: " << exception.what()
                << std::endl;
    }
    return true;
  }

  bool isValidCert(const Botan::X509_Certificate &cert) {

    // TODO : cert 정보는 하드코딩 한 상태 -> 나중에 수정
    /*
    if (validTime(cert) != true)
      return false;
    if (cert.subject_info("Name")[0] != "TUVSR0VSLTE=")
      return false;
    if (cert.subject_info("RFC822")[0] != "contact@gruut.net")
      return false;
    if (cert.subject_info("Organization")[0] != "Gruut Networks")
      return false;
    if (cert.subject_info("Country")[0] != "KR")
      return false;
    if (cert.issuer_info("Name")[0] != "//////////8=")
      return false;
    if (cert.issuer_info("Organization")[0] != "Gruut Networks")
      return false;
    if (Botan::hex_encode(cert.serial_number()) !=
        "EEE11C41D93BB0CAC68FA720914014C3")
      return false;
    */
    return true;
  }

  bool isValidTxInBlock(const json &tx_json) {

    if (Safe::getString(tx_json["type"]) != TXTYPE_CERTIFICATES)
      return false;

    if (!tx_json["content"].is_array())
      return false;

    json content = tx_json["content"];

    for (size_t i = 0; i < content.size(); i += 2) {
      std::string user_id = Safe::getString(content[i]);
      std::string cert_pem = Safe::getString(content[i + 1]);

      // TODO : 나중에 변경
      std::string ca_cert;

      if (!isValidCert(ca_cert, cert_pem))
        return false;

      timestamp_t received_time = static_cast<timestamp_t>(Safe::getTime(tx_json, "time"));
      std::string cert_from_db = getCertificate(user_id, received_time);

      if (!checkDuplicatedCert(cert_pem, cert_from_db))
        return false;
    }

    return true;
  }

  bool checkDuplicatedCert(const std::string &cert_pem, const std::string &cert_from_db) {
    try {
      Botan::X509_Certificate cert = strToCert(cert_pem);
      Botan::X509_Certificate db_cert = strToCert(cert_from_db);

      if (Botan::hex_encode(cert.serial_number()) ==
          Botan::hex_encode(db_cert.serial_number()))
        return false;
    } catch (Botan::Exception &exception) {
      throw;
    }

    return true;
  }

  bool saveCert(const json &tx_json) {
    std::string key, value;

    json content = tx_json["content"];
    if (!content.is_array())
      return false;

    for (size_t c_idx = 0; c_idx < content.size(); c_idx += 2) {
      string user_id_b64 = Safe::getString(content[c_idx]);
      string cert_idx = searchLedger(user_id_b64);

      key = user_id_b64;
      value = (cert_idx.empty()) ? "1" : to_string(stoi(cert_idx) + 1);
      if (!saveLedger(key, value))
        return false;

      key += (cert_idx.empty()) ? "_0" : "_" + cert_idx;
      std::string pem = Safe::getString(content[c_idx + 1]);
      value = parseCert(pem);

      if (value.empty())
        return false;

      if (!saveLedger(key, value))
        return false;
    }

    return true;
  }

  std::string parseCert(const std::string &pem) {

    // User ID에 해당하는 n번째 Certification 저장 (발급일, 만료일, 인증서)
    std::string json_str;

    try {
      Botan::DataSource_Memory cert_datasource(pem);
      Botan::X509_Certificate cert(cert_datasource);

      json tmp_cert = json::array();
      tmp_cert.push_back(
          to_string(Botan::X509_Time(cert.not_before()).time_since_epoch()));
      tmp_cert.push_back(
          to_string(Botan::X509_Time(cert.not_after()).time_since_epoch()));
      tmp_cert.push_back(pem);

      json_str = tmp_cert.dump();
    } catch (...) {
      // do nothing
    }

    return json_str;
  }

  Botan::X509_Certificate strToCert(const std::string &cert_pem_str) {

    try {
      Botan::DataSource_Memory cert_datasource(cert_pem_str);
      Botan::X509_Certificate cert(cert_datasource);
      return cert;

    } catch (Botan::Exception &exception) {
      std::cout << "error on str to cert : " << exception.what() << std::endl;
      throw;
    }
  }

  bool validTime(const Botan::X509_Certificate &cert) {
    Botan::X509_Time t1 = cert.not_before();
    Botan::X509_Time t2 = cert.not_after();

    Botan::X509_Time now = Botan::X509_Time(std::chrono::system_clock::now());

    return (t1.cmp(now) == -1 && t2.cmp(now) == 1);
  }
};
} // namespace gruut
#endif