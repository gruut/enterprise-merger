#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "merkle_tree.hpp"
#include "signature.hpp"
#include "transaction.hpp"
#include "types.hpp"

#include "../services/storage.hpp"
#include "../utils/compressor.hpp"

#include "easy_logging.hpp"
#include "nlohmann/json.hpp"

#include <vector>

using namespace std;

namespace gruut {
class PartialBlock {
public:
  timestamp_type time;
  merger_id_type merger_id;
  local_chain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;

  vector<Transaction> transactions;
};

class Block {
private:
  block_version_type m_version;
  timestamp_type m_time;
  block_id_type m_block_id;
  merger_id_type m_merger_id;
  local_chain_id_type m_chain_id;
  block_height_type m_height;
  transaction_root_type m_tx_root;
  std::vector<Transaction> m_transactions;
  std::vector<sha256> m_merkle_tree_node;
  std::string m_prev_block_hash_b64;
  std::string m_prev_block_id_b64;
  bytes m_signature;
  std::vector<Signature> m_ssigs;
  std::map<std::string, std::string> m_user_certs;
  bytes m_block_raw;
  sha256 m_block_hash;

public:
  Block() { el::Loggers::getLogger("BLOC"); };

  bool initWithParitalBlock(PartialBlock &partial_block,
                            std::vector<sha256> &&merkle_Tree_node = {}) {
    m_time = partial_block.time;
    m_merger_id = partial_block.merger_id;
    m_chain_id = partial_block.chain_id;
    m_height = partial_block.height;
    m_tx_root = partial_block.transaction_root;
    m_transactions = partial_block.transactions;

    if (merkle_Tree_node.empty())
      m_merkle_tree_node = calcMerkleTreeNode();
    else
      m_merkle_tree_node = merkle_Tree_node;

    extractUserCertsIf();

    return true;
  }

  bool initWithMessageJson(nlohmann::json &msg_body) {

    bytes block_raw_bytes = Safe::getBytesFromB64(msg_body, "blockraw");

    nlohmann::json block_header_json = parseBlockRaw(block_raw_bytes);
    if (block_header_json.empty())
      return false;

    m_time = Safe::getTime(block_header_json, "time");
    m_merger_id =
        Safe::getBytesFromB64<merger_id_type>(block_header_json, "mID");
    m_chain_id = TypeConverter::base64ToArray<CHAIN_ID_TYPE_SIZE>(
        Safe::getString(block_header_json, "cID"));
    m_height = Safe::getInt(block_header_json, "hgt");
    m_tx_root =
        Safe::getBytesFromB64<transaction_root_type>(block_header_json, "txrt");
    m_prev_block_hash_b64 = Safe::getString(block_header_json, "prevH");
    m_prev_block_id_b64 = Safe::getString(block_header_json, "prevbID");
    m_block_id = Safe::getBytesFromB64<block_id_type>(block_header_json, "bID");

    if (!setSupportSigs(block_header_json["SSig"]))
      return false;

    if (!setTransactions(msg_body["tx"]))
      return false;

    m_merkle_tree_node = calcMerkleTreeNode();
    m_user_certs = extractUserCertsIf();

    m_block_raw = block_raw_bytes;
    m_block_hash = Sha256::hash(block_raw_bytes);
    m_signature = parseBlockSignature(block_raw_bytes);

    return true;
  }

  bool setTransactions(std::vector<Transaction> &transactions) {
    m_transactions = transactions;
    return true;
  }

  bool setTransactions(nlohmann::json &txs_json) {
    if (!txs_json.is_array()) {
      return false;
    }

    m_transactions.clear();
    for (auto &each_tx_json : txs_json) {
      Transaction each_tx;
      each_tx.setJson(each_tx_json);
      m_transactions.emplace_back(each_tx);
    }

    return true;
  }

  bool setSupportSigs(std::vector<Signature> &ssigs) {
    if (ssigs.empty())
      return false;
    m_ssigs = ssigs;
    return true;
  }

  bool setSupportSigs(nlohmann::json &ssigs) {
    if (!ssigs.is_array())
      return false;

    m_ssigs.clear();
    for (auto &each_ssig : ssigs) {
      Signature tmp;
      tmp.signer_id = Safe::getBytesFromB64<signer_id_type>(each_ssig, "sID");
      tmp.signer_signature =
          Safe::getBytesFromB64<signature_type>(each_ssig, "sig");
      m_ssigs.emplace_back(tmp);
    }

    return true;
  }

  void linkPreviousBlock() {
    auto storage = Storage::getInstance();

    m_version = config::DEFAULT_VERSION;
    std::tuple<string, string, size_t> latest_block_info =
        storage->findLatestBlockBasicInfo();

    if (std::get<0>(latest_block_info).empty()) { // this is genesis block
      m_prev_block_id_b64 = config::GENESIS_BLOCK_PREV_ID_B64;
      m_prev_block_hash_b64 = config::GENESIS_BLOCK_PREV_HASH_B64;
    } else {
      m_prev_block_id_b64 = std::get<0>(latest_block_info);
      m_prev_block_hash_b64 =
          TypeConverter::encodeBase64(std::get<1>(latest_block_info));
    }

    BytesBuilder block_id_builder;
    block_id_builder.append(m_chain_id);
    block_id_builder.append(m_height);
    block_id_builder.append(m_merger_id);
    m_block_id =
        static_cast<block_id_type>(Sha256::hash(block_id_builder.getBytes()));
  }

  void refreshBlockRaw() {
    nlohmann::json block_header = getBlockHeaderJson();

    bytes compressed_block_header;

    switch (config::DEFAULT_COMPRESSION_TYPE) {
    case CompressionAlgorithmType::LZ4:
      compressed_block_header = Compressor::compressData(
          TypeConverter::stringToBytes(block_header.dump()));
      break;
    case CompressionAlgorithmType::MessagePack:
      compressed_block_header = json::to_msgpack(block_header);
      break;
    case CompressionAlgorithmType::CBOR:
      compressed_block_header = json::to_cbor(block_header);
      break;
    default:
      compressed_block_header =
          TypeConverter::stringToBytes(block_header.dump());
    }
    // now, we cannot change block_raw any more

    auto header_length =
        static_cast<header_length_type>(compressed_block_header.size() + 5);

    BytesBuilder block_raw_builder;
    block_raw_builder.append(
        static_cast<uint8_t>(config::DEFAULT_COMPRESSION_TYPE)); // 1-byte
    block_raw_builder.append(header_length, 4);                  // 4-bytes
    block_raw_builder.append(compressed_block_header);

    auto setting = Setting::getInstance();

    string rsa_sk = setting->getMySK();
    string rsa_sk_pass = setting->getMyPass();
    m_signature =
        RSA::doSign(rsa_sk, block_raw_builder.getBytes(), true, rsa_sk_pass);

    block_raw_builder.append(m_signature);

    m_block_raw = block_raw_builder.getBytes();

    m_block_hash = Sha256::hash(m_block_raw);
  }

  bytes getBlockRaw() {
    if (m_block_raw.empty()) {
      refreshBlockRaw();
    }
    return m_block_raw;
  }

  std::vector<transaction_id_type> getTxIds() {
    std::vector<transaction_id_type> ret_txids;
    for (auto &each_tx : m_transactions) {
      ret_txids.emplace_back(each_tx.getId());
    }
    return ret_txids;
  }

  nlohmann::json getBlockHeaderJson() {

    nlohmann::json block_header;

    block_header["ver"] = to_string(m_version);
    block_header["cID"] = TypeConverter::encodeBase64(m_chain_id);
    block_header["prevH"] = m_prev_block_hash_b64;
    block_header["prevbID"] = m_prev_block_id_b64;
    block_header["bID"] = TypeConverter::encodeBase64(m_block_id);
    block_header["time"] = to_string(m_time); // important!!
    block_header["hgt"] = to_string(m_height);
    block_header["txrt"] = TypeConverter::encodeBase64(m_tx_root);

    for (auto &each_tx : m_transactions) {
      block_header["txids"].push_back(
          TypeConverter::encodeBase64(each_tx.getId()));
    }

    for (auto &ssig : m_ssigs) {
      block_header["SSig"].push_back(nlohmann::json(
          {{"sID", TypeConverter::encodeBase64(ssig.signer_id)},
           {"sig", TypeConverter::encodeBase64(ssig.signer_signature)}}));
    }

    block_header["mID"] = TypeConverter::encodeBase64(m_merger_id);

    return block_header;
  }

  nlohmann::json getBlockBodyJson() {

    std::vector<std::string> mtree_node_b64;
    for (size_t i = 0; i < m_transactions.size(); ++i) {
      mtree_node_b64.push_back(
          TypeConverter::encodeBase64(m_merkle_tree_node[i]));
    }

    std::vector<json> txs;
    for (auto &each_tx : m_transactions) {
      txs.push_back(each_tx.getJson());
    }

    nlohmann::json block_body;
    block_body["mtree"] = mtree_node_b64;
    block_body["txCnt"] = to_string(m_transactions.size());
    block_body["tx"] = txs;
    return block_body;
  }

  size_t getHeight() { return m_height; }

  size_t getNumTransactions() { return m_transactions.size(); }

  block_id_type getBlockId() { return m_block_id; }

  sha256 getHash() { return m_block_hash; }

  std::string getHashB64() { return TypeConverter::encodeBase64(m_block_hash); }

  std::string getPrevHashB64() { return m_prev_block_hash_b64; }

  bool isValid() {

    // step 1 - check merkle tree
    if (m_tx_root != m_merkle_tree_node.back()) {
      CLOG(ERROR, "BLOC") << "Invalid Merkle-tree root";
      return false;
    }

    // step 2 - check support signatures
    auto storage = Storage::getInstance();

    bytes ssig_msg_after_sid = getSupportSigMessageAfterSid();

    for (auto &each_ssig : m_ssigs) {
      BytesBuilder ssig_msg_builder;
      ssig_msg_builder.append(each_ssig.signer_id);
      ssig_msg_builder.append(ssig_msg_after_sid);

      std::string user_id_b64 =
          TypeConverter::encodeBase64(each_ssig.signer_id);
      std::string user_pk_pem;

      auto it_map = m_user_certs.find(user_id_b64);
      if (it_map != m_user_certs.end()) {
        user_pk_pem = it_map->second;
      } else {
        user_pk_pem = storage->findCertificate(user_id_b64, m_time);
      }

      if (user_pk_pem.empty()) {
        CLOG(ERROR, "BLOC") << "No suitable user certificate";
        return false;
      }

      if (!RSA::doVerify(user_pk_pem, ssig_msg_builder.getBytes(),
                         each_ssig.signer_signature, true)) {
        CLOG(ERROR, "BLOC") << "Invalid support signature";
        return false;
      }
    }

    // step 3 - check merger's signature

    auto setting = Setting::getInstance();

    std::string merger_pk_pem;

    std::vector<MergerInfo> merger_list = setting->getMergerInfo();

    for (auto &merger : merger_list) {
      if (merger.id == m_merger_id) {
        merger_pk_pem = merger.cert;
        break;
      }
    }

    if (merger_pk_pem.empty()) {
      CLOG(ERROR, "BLOC") << "No suitable merger certificate";
    }

    if (!RSA::doVerify(merger_pk_pem, m_block_raw, m_signature, true)) {
      CLOG(ERROR, "BLOC") << "Invalid merger signature";
      return false;
    }

    return true;
  }

private:
  void init() {}

  bytes genBlockRaw() {

    bytes compressed_json;

    nlohmann::json block_header = getBlockHeaderJson();

    switch (config::DEFAULT_COMPRESSION_TYPE) {
    case CompressionAlgorithmType::LZ4:
      compressed_json = Compressor::compressData(
          TypeConverter::stringToBytes(block_header.dump()));
      break;
    case CompressionAlgorithmType::MessagePack:
      compressed_json = json::to_msgpack(block_header);
      break;
    case CompressionAlgorithmType::CBOR:
      compressed_json = json::to_cbor(block_header);
      break;
    default:
      compressed_json = TypeConverter::stringToBytes(block_header.dump());
    }
    // now, we cannot change block_raw any more

    auto header_length =
        static_cast<header_length_type>(compressed_json.size() + 5);

    BytesBuilder block_raw_builder;
    block_raw_builder.append(
        static_cast<uint8_t>(config::DEFAULT_COMPRESSION_TYPE)); // 1-byte
    block_raw_builder.append(header_length, 4);                  // 4-bytes
    block_raw_builder.append(compressed_json);

    return block_raw_builder.getBytes();
  }

  bytes getSupportSigMessageAfterSid() {
    BytesBuilder ssig_msg_after_sid_builder;
    ssig_msg_after_sid_builder.append(m_time);
    ssig_msg_after_sid_builder.append(m_merger_id);
    ssig_msg_after_sid_builder.append(m_chain_id);
    ssig_msg_after_sid_builder.append(m_height);
    ssig_msg_after_sid_builder.append(m_tx_root);
    return ssig_msg_after_sid_builder.getBytes();
  }

  std::vector<sha256> calcMerkleTreeNode() {
    std::vector<sha256> merkle_tree_node;
    std::vector<sha256> tx_digests;

    for (auto &each_tx : m_transactions) {
      tx_digests.emplace_back(each_tx.getDigest());
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    merkle_tree_node = merkle_tree.getMerkleTree();
    return merkle_tree_node;
  }

  std::map<std::string, std::string> extractUserCertsIf() {
    std::map<std::string, std::string> ret_map;
    for (auto &each_tx : m_transactions) {
      std::map<std::string, std::string> tx_user_certs = each_tx.getCertsIf();
      if (!tx_user_certs.empty())
        ret_map.insert(tx_user_certs.begin(), tx_user_certs.end());
    }

    return ret_map;
  }

  bytes parseBlockSignature(bytes &block_raw_bytes) {
    if (block_raw_bytes.size() <= 5) {
      return bytes();
    }

    size_t header_end = static_cast<size_t>(
        block_raw_bytes[1] << 24 | block_raw_bytes[2] << 16 |
        block_raw_bytes[3] << 8 | block_raw_bytes[4]);

    if (header_end >= block_raw_bytes.size())
      return bytes();

    return bytes(block_raw_bytes.begin() + header_end, block_raw_bytes.end());
  }

  nlohmann::json parseBlockRaw(bytes &block_raw_bytes) {

    el::Loggers::getLogger("BLOC");

    nlohmann::json block_header_json;

    if (block_raw_bytes.size() <= 5) {
      return block_header_json;
    }

    size_t header_end = static_cast<size_t>(
        block_raw_bytes[1] << 24 | block_raw_bytes[2] << 16 |
        block_raw_bytes[3] << 8 | block_raw_bytes[4]);

    std::string block_header_comp(block_raw_bytes.begin() + 5,
                                  block_raw_bytes.begin() + header_end);

    switch (static_cast<CompressionAlgorithmType>(block_raw_bytes[0])) {
    case CompressionAlgorithmType::LZ4: {
      block_header_json =
          Safe::parseJson(Compressor::decompressData(block_header_comp));
      break;
    }
    case CompressionAlgorithmType::MessagePack: {
      block_header_json = json::from_msgpack(block_header_comp);
      break;
    }
    case CompressionAlgorithmType::CBOR: {
      block_header_json = json::from_cbor(block_header_comp);
      break;
    }
    case CompressionAlgorithmType::NONE: {
      block_header_json = Safe::parseJson(block_header_comp);
      break;
    }
    default:
      CLOG(ERROR, "BLOC") << "Unknown compress type";
      return block_header_json;
    }

    if (block_header_json.empty())
      CLOG(ERROR, "BLOC") << "Invalid JSON structure";

    return block_header_json;
  }
};

} // namespace gruut
#endif
