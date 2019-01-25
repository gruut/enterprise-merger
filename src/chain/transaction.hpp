#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "nlohmann/json.hpp"

#include "../utils/bytes_builder.hpp"
#include "../utils/ecdsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/sha256.hpp"
#include "../utils/type_converter.hpp"
#include "types.hpp"

#include <array>
#include <string>
#include <vector>

#include <iostream>

namespace gruut {
class Transaction {
private:
  tx_id_type m_transaction_id;
  timestamp_t m_sent_time;
  requestor_id_type m_requestor_id;
  TransactionType m_transaction_type;
  signature_type m_signature;
  std::vector<content_type> m_content_list;

public:
  Transaction()
      : m_sent_time(0), m_transaction_id({0x00}),
        m_transaction_type(TransactionType::UNKNOWN) {}

  Transaction(tx_id_type transaction_id, timestamp_t sent_time,
              requestor_id_type &requestor_id, TransactionType transaction_type,
              signature_type &signature,
              std::vector<content_type> &content_list)
      : m_transaction_id(transaction_id), m_sent_time(sent_time),
        m_requestor_id(requestor_id), m_transaction_type(transaction_type),
        m_signature(signature), m_content_list(content_list) {}

  bool setJson(json &tx_json) {

    auto new_txid_bytes = Safe::getBytesFromB64(tx_json, "txid");

    if (new_txid_bytes.size() != 32)
      return false;

    setId(
        TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(new_txid_bytes));
    setTime(Safe::getTime(tx_json, "time"));
    setRequestorId(Safe::getBytesFromB64<id_type>(tx_json, "rID"));
    setTransactionType(strToTxType(Safe::getString(tx_json, "type")));

    if (tx_json["content"].is_array())
      setContentsFromJson(tx_json["content"]);

    setSignature(Safe::getBytesFromB64(tx_json, "rSig"));

    return true;
  }

  json getJson() {
    return json({{"txid", TypeConverter::encodeBase64(m_transaction_id)},
                 {"time", to_string(m_sent_time)},
                 {"rID", TypeConverter::encodeBase64(m_requestor_id)},
                 {"type", txTypeToStr(m_transaction_type)},
                 {"rSig", TypeConverter::encodeBase64(m_signature)},
                 {"content", m_content_list}});
  }

  template <typename T = tx_id_type> void setId(T &&transaction_id) {
    m_transaction_id = transaction_id;
  }

  tx_id_type getId() { return m_transaction_id; }

  std::string getIdB64() {
    return TypeConverter::encodeBase64(m_transaction_id);
  }

  std::string getIdStr(){
    return TypeConverter::arrayToString<TRANSACTION_ID_TYPE_SIZE>(m_transaction_id);
  }

  void setTime(timestamp_t sent_time) { m_sent_time = sent_time; }

  timestamp_t getTime() { return m_sent_time; }

  template <typename T = requestor_id_type>
  void setRequestorId(T &&requestor_id) {
    m_requestor_id = requestor_id;
  }

  void setTransactionType(TransactionType transaction_type) {
    m_transaction_type = transaction_type;
  }

  TransactionType getTransactionType() { return m_transaction_type; }

  template <typename T = signature_type> void setSignature(T &&signature) {
    m_signature = signature;
  }

  template <typename T = json> void setContentsFromJson(T &&content_list) {
    m_content_list.clear();
    if (content_list.is_array()) {
      for (auto &cont_item : content_list) {
        m_content_list.emplace_back(Safe::getString(cont_item));
      }
    }
  }

  template <typename T = std::vector<content_type>>
  void setContents(T &&content_list) {
    m_content_list = content_list;
  }

  std::map<std::string, std::string> getCertsIf() {
    std::map<std::string, std::string> ret_certs;
    if (m_transaction_type == TransactionType::CERTIFICATES) {
      for (size_t i = 0; i < m_content_list.size(); i += 2) {
        ret_certs.insert({m_content_list[i], m_content_list[i + 1]});
      }
    }

    return ret_certs;
  }

  id_type getRequesterId() { return m_requestor_id; }

  void genNewTxId() { setId(buildTxId()); }

  template <typename T = std::string> bool isValid(T &&pk_pem) {

    if (m_content_list.empty() || m_signature.empty() || pk_pem.empty()) {
      return false;
    }

    if (m_transaction_id != buildTxId()) {
      return false;
    }

    return ECDSA::doVerify(pk_pem, getBeforeDigestByte(), m_signature);
  }

  template <typename T = std::string>
  void refreshSignature(T &&pem_sk, T &&pem_pass) {
    m_signature = ECDSA::doSign(pem_sk, getBeforeDigestByte(), pem_pass);
  }

  hash_t getDigest() {
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

  template <typename T = std::string>
  TransactionType strToTxType(std::string &&tx_type_str) {
    TransactionType ret_type;
    if (tx_type_str == TXTYPE_DIGESTS)
      ret_type = TransactionType::DIGESTS;
    else if (tx_type_str == TXTYPE_CERTIFICATES)
      ret_type = TransactionType::CERTIFICATES;
    else if (tx_type_str == TXTYPE_IMMORTALSMS)
      ret_type = TransactionType::IMMORTALSMS;
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

  tx_id_type buildTxId() {

    BytesBuilder txid_builder;
    txid_builder.append(m_sent_time);
    txid_builder.append(m_requestor_id);
    txid_builder.append(txTypeToStr(m_transaction_type));

    for (auto &content : m_content_list) {
      txid_builder.append(content);
    }

    return static_cast<tx_id_type>(
        TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(
            Sha256::hash(txid_builder.getBytes())));
  }
};

} // namespace gruut
#endif
