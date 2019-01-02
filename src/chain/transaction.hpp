#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "../services/setting.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/sha256.hpp"
#include "../utils/type_converter.hpp"
#include "types.hpp"

#include "../../include/nlohmann/json.hpp"
#include <boost/assert.hpp>

#include <array>
#include <string>
#include <vector>

#include <iostream>

namespace gruut {
class Transaction {
private:
  transaction_id_type m_transaction_id;
  timestamp_type m_sent_time;
  requestor_id_type m_requestor_id;
  TransactionType m_transaction_type;
  signature_type m_signature;
  std::vector<content_type> m_content_list;

public:
  Transaction()
      : m_sent_time(0), m_transaction_id({0x00}),
        m_transaction_type(TransactionType::UNKNOWN) {}

  Transaction(transaction_id_type transaction_id, timestamp_type sent_time,
              requestor_id_type &requestor_id, TransactionType transaction_type,
              signature_type &signature,
              std::vector<content_type> &content_list)
      : m_transaction_id(transaction_id), m_sent_time(sent_time),
        m_requestor_id(requestor_id), m_transaction_type(transaction_type),
        m_signature(signature), m_content_list(content_list) {}

  bool setJson(nlohmann::json &tx_json) {

    auto new_txid_bytes = Safe::getBytesFromB64(tx_json, "txid");

    BOOST_ASSERT_MSG(new_txid_bytes.size() == 32,
                     "The size of the transaction is not 32 bytes");

    setId(
        TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(new_txid_bytes));
    setTime(Safe::getTime(tx_json, "time"));
    setRequestorId(Safe::getBytesFromB64<id_type>(tx_json, "rID"));
    setTransactionType(strToTxType(Safe::getString(tx_json, "type")));

    if (tx_json["content"].is_array())
      setContents(tx_json["content"]);

    setSignature(Safe::getBytesFromB64(tx_json, "rSig"));

    return true;
  }

  nlohmann::json getJson() {
    return nlohmann::json(
        {{"txid", TypeConverter::encodeBase64(m_transaction_id)},
         {"time", to_string(m_sent_time)},
         {"rID", TypeConverter::encodeBase64(m_requestor_id)},
         {"type", txTypeToStr(m_transaction_type)},
         {"rSig", TypeConverter::encodeBase64(m_signature)},
         {"content", m_content_list}});
  }

  inline void setId(transaction_id_type transaction_id) {
    m_transaction_id = transaction_id;
  }

  inline transaction_id_type getId() { return m_transaction_id; }

  inline void setTime(timestamp_type sent_time) { m_sent_time = sent_time; }

  inline void setRequestorId(requestor_id_type &&requestor_id) {
    m_requestor_id = requestor_id;
  }

  inline void setRequestorId(requestor_id_type &requestor_id) {
    m_requestor_id = requestor_id;
  }

  inline void setTransactionType(TransactionType transaction_type) {
    m_transaction_type = transaction_type;
  }

  inline void setSignature(signature_type &&signature) {
    m_signature = signature;
  }

  inline void setSignature(signature_type &signature) {
    m_signature = signature;
  }

  inline void setContents(nlohmann::json &content_list) {
    m_content_list.clear();
    if (content_list.is_array()) {
      for (auto &cont_item : content_list) {
        m_content_list.emplace_back(Safe::getString(cont_item));
      }
    }
  }
  inline void setContents(std::vector<content_type> &&content_list) {
    setContents(content_list);
  }

  inline void setContents(std::vector<content_type> &content_list) {
    m_content_list = content_list;
  }

  std::map<std::string, std::string> getCertsIf() {
    std::map<std::string, std::string> ret_certs;
    if (m_transaction_type == TransactionType::CERTIFICATE) {
      for (size_t i = 0; i < m_content_list.size(); i += 2) {
        ret_certs.insert({m_content_list[i], m_content_list[i + 1]});
      }
    }

    return ret_certs;
  }

  bool isValid(const std::string &pk_pem_instant = "") {

    if (m_content_list.empty() || m_signature.empty()) {
      return false;
    }

    std::string pk_pem = pk_pem_instant;

    if (pk_pem.empty()) {
      std::vector<ServiceEndpointInfo> service_endpoints =
          Setting::getInstance()->getServiceEndpointInfo();
      for (auto &srv_point : service_endpoints) {
        if (srv_point.id == m_requestor_id) {
          pk_pem = srv_point.cert;
          break;
        }
      }
    }

    if (pk_pem.empty()) { // not in service endpoint
      std::vector<MergerInfo> mergers = Setting::getInstance()->getMergerInfo();
      for (auto &merger : mergers) {
        if (merger.id == m_requestor_id) {
          pk_pem = merger.cert;
          break;
        }
      }
    }

    if (pk_pem.empty()) {
      return false;
    }

    return RSA::doVerify(pk_pem, getBeforeDigestByte(), m_signature, true);
  }

  void refreshSignature(const std::string &pem_sk_instant = "",
                        const std::string &pem_pass_instant = "") {

    std::string pem_sk, pem_pass;

    if (pem_sk_instant.empty()) {
      auto setting = Setting::getInstance();
      pem_sk = setting->getMySK();
      pem_pass = setting->getMyPass();
    } else {
      pem_sk = pem_sk_instant;
      pem_pass = pem_pass_instant;
    }

    m_signature = RSA::doSign(pem_sk, getBeforeDigestByte(), true, pem_pass);
  }

  sha256 getDigest() {
    bytes msg = getBeforeDigestByte();
    msg.insert(msg.end(), m_signature.begin(), m_signature.end());
    return Sha256::hash(msg);
  }

private:
  std::string txTypeToStr(TransactionType transaction_type) {

    std::string ret_str = "UNKNOWN";

    auto it_map = TX_TYPE_TO_STRING.find(transaction_type);
    if (it_map != TX_TYPE_TO_STRING.end()) {
      ret_str = it_map->second;
    }

    return ret_str;
  }

  TransactionType strToTxType(std::string &&tx_type_str) {
    return strToTxType(tx_type_str);
  }

  TransactionType strToTxType(std::string &tx_type_str) {
    TransactionType ret_type;
    if (tx_type_str == TXTYPE_DIGESTS)
      ret_type = TransactionType::DIGESTS;
    else if (tx_type_str == TXTYPE_CERTIFICATES)
      ret_type = TransactionType::CERTIFICATE;
    else
      ret_type = TransactionType::UNKNOWN;

    return ret_type;
  }

  bytes getBeforeDigestByte() {
    BytesBuilder msg_builder;
    msg_builder.append(m_transaction_id);
    msg_builder.append(m_sent_time);
    msg_builder.append(m_requestor_id);
    msg_builder.append(txTypeToStr(m_transaction_type));

    for (auto &content : m_content_list) {
      msg_builder.append(content);
    }

    return msg_builder.getBytes();
  }
};

} // namespace gruut
#endif
