#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "mem_ledger.hpp"
#include "merkle_tree.hpp"
#include "signature.hpp"
#include "transaction.hpp"
#include "types.hpp"

#include "../services/certificate_pool.hpp"
#include "../services/storage.hpp"
#include "../utils/compressor.hpp"
#include "../utils/ecdsa.hpp"

#include "../ledger/certificate_ledger.hpp"
#include "easy_logging.hpp"
#include "nlohmann/json.hpp"

#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <sstream>

using namespace std;

namespace gruut {
class BasicBlockInfo {
public:
  timestamp_t time;
  merger_id_type merger_id;
  localchain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;
  std::string prev_id_b64;
  std::string prev_hash_b64;

  vector<Transaction> transactions;
};

class Block {
private:
  block_version_type m_version;
  timestamp_t m_time;
  block_id_type m_block_id;
  merger_id_type m_merger_id;
  localchain_id_type m_chain_id;
  block_height_type m_height;
  transaction_root_type m_tx_root;
  hash_t m_prev_block_hash;
  hash_t m_block_hash;
  block_id_type m_prev_block_id;
  bytes m_signature;
  std::vector<Transaction> m_transactions;
  std::vector<hash_t> m_merkle_tree_node;
  std::vector<Signature> m_ssigs;
  std::map<std::string, std::string> m_user_certs;
  bytes m_block_raw;

public:
  Block() { el::Loggers::getLogger("BLOC"); };

  bool initialize(BasicBlockInfo &basic_info,
                  std::vector<hash_t> &&merkle_Tree_node = {}) {
    m_time = basic_info.time;
    m_merger_id = basic_info.merger_id;
    m_chain_id = basic_info.chain_id;
    m_height = basic_info.height;
    m_tx_root = basic_info.transaction_root;
    m_transactions = basic_info.transactions;

    if (merkle_Tree_node.empty())
      m_merkle_tree_node = calcMerkleTreeNode();
    else
      m_merkle_tree_node = merkle_Tree_node;

    m_user_certs = extractUserCertsIf();

    return true;
  }

  bool initialize(storage_block_type &read_block) {
    return initialize(read_block.block_raw, read_block.txs);
  }

  bool initialize(json &msg_body) {
    bytes block_raw_bytes = Safe::getBytesFromB64(msg_body, "blockraw");
    return initialize(block_raw_bytes, msg_body["tx"]);
  }

  bool initialize(bytes &block_raw_bytes, json &block_txs) {

    if (block_raw_bytes.empty())
      return false;

    json block_header_json = getBlockHeader(block_raw_bytes);
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
    m_prev_block_hash = Safe::getBytesFromB64(block_header_json, "prevH");
    m_prev_block_id =
        Safe::getBytesFromB64<block_id_type>(block_header_json, "prevbID");
    m_block_id = Safe::getBytesFromB64<block_id_type>(block_header_json, "bID");

    if (!setSupportSignaturesFromJson(block_header_json["SSig"]))
      return false;

    if (!setTransactions(block_txs))
      return false;

    m_merkle_tree_node = calcMerkleTreeNode();
    m_user_certs = extractUserCertsIf();

    m_block_raw = block_raw_bytes;
    m_block_hash = Sha256::hash(block_raw_bytes);
    m_signature = getBlockSignature(block_raw_bytes);

    return true;
  }

  bool setTransactions(std::vector<Transaction> &transactions) {
    m_transactions = transactions;
    return true;
  }

  bool setTransactions(json &txs_json) {
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

  bool setSupportSignatures(std::vector<Signature> &ssigs) {
    if (ssigs.empty())
      return false;
    m_ssigs = ssigs;
    return true;
  }

  bool setSupportSignaturesFromJson(json &ssigs) {
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

  template <typename T = std::string>
  void
  linkPreviousBlock(T &&last_id_b64 = config::GENESIS_BLOCK_PREV_ID_B64,
                    T &&last_hash_b64 = config::GENESIS_BLOCK_PREV_HASH_B64) {

    m_version = config::DEFAULT_VERSION;
    m_prev_block_id = TypeConverter::decodeBase64(last_id_b64);
    m_prev_block_hash = TypeConverter::decodeBase64(last_hash_b64);

    BytesBuilder block_id_builder;
    block_id_builder.append(m_chain_id);
    block_id_builder.append(m_height);
    block_id_builder.append(m_merger_id);
    m_block_id =
        static_cast<block_id_type>(Sha256::hash(block_id_builder.getBytes()));
  }

  void finalize() {
    bytes block_meta_comp_header = generateMetaWithCompHeader();

    auto setting = Setting::getInstance();

    string ecdsa_sk = setting->getMySK();
    string ecdsa_sk_pass = setting->getMyPass();
    m_signature =
        ECDSA::doSign(ecdsa_sk, block_meta_comp_header, ecdsa_sk_pass);

    BytesBuilder block_raw_builder;
    block_raw_builder.append(block_meta_comp_header);
    block_raw_builder.append(m_signature);

    m_block_raw = block_raw_builder.getBytes();

    m_block_hash = Sha256::hash(m_block_raw);
  }

  bytes getBlockRaw() {
    if (m_block_raw.empty()) {
      finalize();
    }
    return m_block_raw;
  }

  std::vector<tx_id_type> getTxIds() {
    std::vector<tx_id_type> ret_txids;
    for (auto &each_tx : m_transactions) {
      ret_txids.emplace_back(each_tx.getId());
    }
    return ret_txids;
  }

  json getBlockHeaderJson() {

    json block_header;

    block_header["ver"] = to_string(m_version);
    block_header["cID"] = TypeConverter::encodeBase64(m_chain_id);
    block_header["prevH"] = TypeConverter::encodeBase64(m_prev_block_hash);
    block_header["prevbID"] = TypeConverter::encodeBase64(m_prev_block_id);
    block_header["bID"] = TypeConverter::encodeBase64(m_block_id);
    block_header["time"] = to_string(m_time); // important!!
    block_header["hgt"] = to_string(m_height);
    block_header["txrt"] = TypeConverter::encodeBase64(m_tx_root);

    for (auto &each_tx : m_transactions) {
      block_header["txids"].push_back(
          TypeConverter::encodeBase64(each_tx.getId()));
    }

    for (auto &ssig : m_ssigs) {
      block_header["SSig"].push_back(
          json({{"sID", TypeConverter::encodeBase64(ssig.signer_id)},
                {"sig", TypeConverter::encodeBase64(ssig.signer_signature)}}));
    }

    block_header["mID"] = TypeConverter::encodeBase64(m_merger_id);

    return block_header;
  }

  json getBlockBodyJson() {

    std::vector<std::string> mtree_node_b64;
    for (size_t i = 0; i < m_transactions.size(); ++i) {
      mtree_node_b64.push_back(
          TypeConverter::encodeBase64(m_merkle_tree_node[i]));
    }

    std::vector<json> txs;
    for (auto &each_tx : m_transactions) {
      txs.push_back(each_tx.getJson());
    }

    return json({{"mtree", mtree_node_b64},
                 {"txCnt", to_string(m_transactions.size())},
                 {"tx", txs}});
  }

  block_height_type getHeight() { return m_height; }

  size_t getNumTransactions() { return m_transactions.size(); }

  size_t getNumSSigs() { return m_ssigs.size(); }

  timestamp_t getTime() { return m_time; }

  hash_t getHash() { return m_block_hash; }
  hash_t getPrevHash() { return m_prev_block_hash; }
  block_id_type getBlockId() { return m_block_id; }
  block_id_type getPrevBlockId() { return m_prev_block_id; }

  std::string getHashB64() { return TypeConverter::encodeBase64(m_block_hash); }
  std::string getPrevHashB64() {
    return TypeConverter::encodeBase64(m_prev_block_hash);
  }
  std::string getBlockIdB64() {
    return TypeConverter::encodeBase64(m_block_id);
  }
  std::string getPrevBlockIdB64() {
    return TypeConverter::encodeBase64(m_prev_block_id);
  }

  bool isValid() { return (isValidEarly() && isValidLate()); }

  // Support signature cannot be verified unless storage or block itself has
  // suitable certificates Therefore, the verification of support signatures
  // should be delayed until the previous block has been saved.
  bool isValidLate() {
    // step - check support signatures
    bytes ssig_msg_after_sid = getSupportSigMessageCommon();
    CertificateLedger cert_ledger;

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
        user_pk_pem = cert_ledger.getCertificate(
            user_id_b64, m_time); // this is from storage
      }

      if (user_pk_pem.empty()) {
        CLOG(ERROR, "BLOC") << "No suitable user certificate";
        return false;
      }

      if (!ECDSA::doVerify(user_pk_pem, ssig_msg_builder.getBytes(),
                           each_ssig.signer_signature)) {
        CLOG(ERROR, "BLOC") << "Invalid support signature";
        return false;
      }
    }

    return true;
  }

  bool isValidEarly() {

    if (m_block_raw.empty() || m_signature.empty()) {
      CLOG(ERROR, "BLOC") << "Empty blockraw or signature";
      return false;
    }

    // step - check merkle tree
    if (m_tx_root != m_merkle_tree_node.back()) {
      CLOG(ERROR, "BLOC") << "Invalid Merkle-tree root";
      return false;
    }

    // step - transactions

    auto cert_pool = CertificatePool::getInstance();

    for (auto &each_tx : m_transactions) {
      id_type requster_id = each_tx.getRequesterId();
      std::string pk_cert = cert_pool->getCert(requster_id);
      if (!each_tx.isValid(pk_cert)) {
        CLOG(ERROR, "BLOC") << "Invalid transaction";
        return false;
      }
    }

    // step - check merger's signature

    std::string merger_pk_cert = cert_pool->getCert(m_merger_id);

    if (merger_pk_cert.empty()) {
      CLOG(ERROR, "BLOC") << "No suitable merger certificate";
      return false;
    }

    bytes meta_header_raw = getBlockMetaHeaderRaw(m_block_raw);

    if (!ECDSA::doVerify(merger_pk_cert, meta_header_raw, m_signature)) {
      CLOG(ERROR, "BLOC") << "Invalid merger signature";
      return false;
    }

    return true;
  }

private:
  bytes generateMetaWithCompHeader() {

    bytes compressed_json;

    json block_header = getBlockHeaderJson();

    switch (config::DEFAULT_BLOCKRAW_COMP_ALGO) {
    case CompressionAlgorithmType::LZ4: {
      compressed_json = Compressor::compressData(
          TypeConverter::stringToBytes(block_header.dump()));
      break;
    }
    case CompressionAlgorithmType::MessagePack: {
      compressed_json = json::to_msgpack(block_header);
      break;
    }
    case CompressionAlgorithmType::CBOR: {
      compressed_json = json::to_cbor(block_header);
      break;
    }
    default:
      compressed_json = TypeConverter::stringToBytes(block_header.dump());
    }
    // now, we cannot change block_raw any more

    auto header_end =
        static_cast<header_length_type>(compressed_json.size() + 5);

    BytesBuilder block_raw_builder;
    block_raw_builder.append(
        static_cast<uint8_t>(config::DEFAULT_BLOCKRAW_COMP_ALGO)); // 1-byte
    block_raw_builder.append(header_end, 4);                       // 4-bytes
    block_raw_builder.append(compressed_json);

    return block_raw_builder.getBytes();
  }

  bytes getSupportSigMessageCommon() {
    BytesBuilder ssig_msg_common_builder;
    ssig_msg_common_builder.append(m_time);
    ssig_msg_common_builder.append(m_merger_id);
    ssig_msg_common_builder.append(m_chain_id);
    ssig_msg_common_builder.append(m_height);
    ssig_msg_common_builder.append(m_tx_root);
    return ssig_msg_common_builder.getBytes();
  }

  std::vector<hash_t> calcMerkleTreeNode() {
    std::vector<hash_t> merkle_tree_node;
    std::vector<hash_t> tx_digests;

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

  size_t getHeaderEnd(bytes &block_raw_bytes) {
    if (block_raw_bytes.size() <= 5) {
      return 0;
    }

    auto header_end = static_cast<size_t>(
        block_raw_bytes[1] << 24 | block_raw_bytes[2] << 16 |
        block_raw_bytes[3] << 8 | block_raw_bytes[4]);

    if (header_end >= block_raw_bytes.size())
      return 0;

    return header_end;
  }

  bytes getBlockMetaHeaderRaw(bytes &block_raw_bytes) {

    size_t header_end = getHeaderEnd(block_raw_bytes);

    if (header_end == 0) {
      CLOG(ERROR, "BLOC") << "Empty header end";
      return bytes();
    }

    return bytes(block_raw_bytes.begin(), block_raw_bytes.begin() + header_end);
  }

  bytes getBlockSignature(bytes &block_raw_bytes) {
    size_t header_end = getHeaderEnd(block_raw_bytes);

    if (header_end == 0) {
      CLOG(ERROR, "BLOC") << "Empty header end";
      return bytes();
    }

    return bytes(block_raw_bytes.begin() + header_end, block_raw_bytes.end());
  }

  json getBlockHeader(bytes &block_raw_bytes) {

    if (block_raw_bytes.empty()) {
      CLOG(ERROR, "BLOC") << "Empty bytes";
      return json::object();
    }

    size_t header_end = getHeaderEnd(block_raw_bytes);

    if (header_end == 0) {
      CLOG(ERROR, "BLOC") << "Empty header end";
      return json::object();
    }

    std::string block_header_comp(block_raw_bytes.begin() + 5,
                                  block_raw_bytes.begin() + header_end);

    auto compression_algo =
        static_cast<CompressionAlgorithmType>(block_raw_bytes[0]);

    json block_header_json;

    switch (compression_algo) {
    case CompressionAlgorithmType::LZ4: {
      block_header_json =
          Safe::parseJson(Compressor::decompressData(block_header_comp));
      break;
    }
    case CompressionAlgorithmType::MessagePack: {
      try {
        block_header_json = json::from_msgpack(block_header_comp);
      } catch (json::exception &e) {
        CLOG(ERROR, "BLOC") << e.what();
      }
      break;
    }
    case CompressionAlgorithmType::CBOR: {
      try {
        block_header_json = json::from_cbor(block_header_comp);
      } catch (json::exception &e) {
        CLOG(ERROR, "BLOC") << e.what();
      }
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
      CLOG(ERROR, "BLOC") << "Failed decompression (" << (int)compression_algo
                          << ")";

    return block_header_json;
  }
};

class BlockSerialize {
private:
  friend class boost::serialization::access;
  std::string block_raw_str;
  std::string block_body_str;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &block_raw_str;
    ar &block_body_str;
  }

public:
  BlockSerialize(){};
  BlockSerialize(Block &block)
      : block_raw_str(TypeConverter::bytesToString(block.getBlockRaw())),
        block_body_str(block.getBlockBodyJson().dump()) {}

  bytes getUnresolvedBlockRaw() {
    return TypeConverter::stringToBytes(block_raw_str);
  }
  json getUnresolvedBlockBody() { return Safe::parseJson(block_body_str); }
};

} // namespace gruut
#endif
