#ifndef GRUUT_ENTERPRISE_MERGER_CERTIFICATE_HPP
#define GRUUT_ENTERPRISE_MERGER_CERTIFICATE_HPP

#include "../services/storage.hpp"
#include "../services/setting.hpp"
#include "../utils/safe.hpp"
#include "ledger.hpp"

#include "easy_logging.hpp"

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
private:
  std::unique_ptr<Botan::X509_Certificate> m_ca_cert;
public:
  CertificateLedger() {
    setPrefix("C");
    el::Loggers::getLogger("CERT");
    loadCACert();
  }

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
        std::string latest_cert = searchLedger(user_id_b64 + "_" + to_string(num_certs - 1));

        json latest_cert_json = Safe::parseJsonAsArray(latest_cert);
        if (!latest_cert_json.empty())
          cert = Safe::getString(latest_cert_json[2]);
      } else {

        timestamp_t max_start_date = 0;
        for (int i = 0; i < num_certs; ++i) {
          std::string nth_cert = searchLedger(user_id_b64 + "_" + to_string(i));

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
    } else {
      CLOG(INFO, "CERT") << "No certificate for [" << user_id_b64 << "]";
    }

    return cert;
  }

  std::string getCertificate(const signer_id_type &user_id,
                             const timestamp_t &at_this_time = 0) {
    std::string user_id_b64 = TypeConverter::encodeBase64(user_id);
    return getCertificate(user_id_b64);
  }

private:

  void loadCACert(){
    try {
      Botan::X509_Certificate ca_cert = pemToX509(Setting::getInstance()->getGruutAuthorityInfo().cert);
      m_ca_cert.reset(new Botan::X509_Certificate(ca_cert));

      if (!m_ca_cert->is_CA_cert()) {
        CLOG(ERROR, "CERT") << "X509_CA: This certificate is not for CA";
      }
    }
    catch (...){
      m_ca_cert.reset(nullptr);
      CLOG(ERROR, "CERT") << "Failed to load CA's certificate! (Exception)";
    }
  }

  bool isValidCert(const std::string &cert_pem_str) {
    if(m_ca_cert == nullptr)
      return false;

    try {
      auto&& cert = pemToX509(cert_pem_str);

      Botan::ECDSA_PublicKey pub_key(
        cert.subject_public_key()->algorithm_identifier(),
        cert.subject_public_key()->public_key_bits());

      if (!m_ca_cert->check_signature(pub_key)) {
        return false;
      }

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

    } catch (Botan::Exception &exception) {
      CLOG(ERROR, "CERT") << "Error on PEM to ECDSA PK: " << exception.what();
      return false;
    }
  }

  bool isValidTxInBlock(const json &tx_json) {

    if (Safe::getString(tx_json,"type") != TXTYPE_CERTIFICATES)
      return false;

    if (!tx_json["content"].is_array())
      return false;

    auto& content = tx_json["content"];

    for (size_t i = 0; i < content.size(); i += 2) {
      std::string user_id = Safe::getString(content[i]);
      std::string user_pem = Safe::getString(content[i + 1]);

      if(user_pem.empty() || !isValidCert(user_pem))
        return false;

      timestamp_t recv_time = Safe::getTime(tx_json, "time");
      std::string pem_in_db = getCertificate(user_id, recv_time);

      if (pem_in_db.empty() || !isDuplicatedCert(user_pem, pem_in_db))
        return true;
    }

    return true;
  }

  bool isDuplicatedCert(const std::string &user_pem, const std::string &pem_in_db) {

    if(user_pem.empty() || pem_in_db.empty() || user_pem == pem_in_db)
      return true;

    try {
      auto&& cert = pemToX509(user_pem);
      auto&& cert_in_db = pemToX509(pem_in_db);
      return (cert.serial_number() == cert_in_db.serial_number());
    } catch (...) {
      return true;
    }
  }

  bool saveCert(const json &tx_json) {
    std::string key, value;

    auto& content = tx_json["content"];
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

  Botan::X509_Certificate pemToX509(const std::string &cert_pem_str) {

    try {
      Botan::DataSource_Memory cert_datasource(cert_pem_str);
      Botan::X509_Certificate x509_cert(cert_datasource);
      return x509_cert;
    } catch (Botan::Exception &exception) {
      CLOG(ERROR, "CERT") << "Error on string to X509_Certificate - " << exception.what();
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