#pragma once

#include "../../include/nlohmann/json.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/sha256.hpp"
#include "easy_logging.hpp"
#include "setting.hpp"
#include "storage.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
namespace gruut {
class BlockValidator {

public:
  static nlohmann::json getBlockHeaderJson(bytes &block_raw_bytes) {

    el::Loggers::getLogger("BVAL");

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
      CLOG(ERROR, "BVAL") << "Unknown compress type";
      return block_header_json;
    }

    if (block_header_json.empty())
      CLOG(ERROR, "BVAL") << "Invalid JSON structure";

    return block_header_json;
  }

  static bool validateAndGetTree(nlohmann::json &block_header_json,
                                 nlohmann::json &txs_json,
                                 std::vector<sha256> &merkle_tree_nodes,
                                 std::vector<transaction_id_type> &txids) {

    el::Loggers::getLogger("BVAL");

    auto setting = Setting::getInstance();
    std::vector<MergerInfo> mergers = setting->getMergerInfo();

    if (txs_json.empty() || !txs_json.is_array()) {
      CLOG(ERROR, "BVAL") << "TX is empty or not array";
      return false;
    }

    txids.clear();

    std::map<std::string, std::string> user_cert_map;

    std::vector<sha256> tx_digests;

    for (auto &each_tx_json : txs_json) {
      Transaction each_tx;
      each_tx.setJson(each_tx_json);
      if (each_tx.isValid()) {
        tx_digests.emplace_back(each_tx.getDigest());
        std::map<std::string, std::string> tx_user_cert_map =
            each_tx.getCertsIf();
        user_cert_map.insert(tx_user_cert_map.begin(), tx_user_cert_map.end());
      } else {
        CLOG(ERROR, "BVAL") << "Block has an invalid TX";
        return false;
      }
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    merkle_tree_nodes = merkle_tree.getMerkleTree();

    if (Safe::getString(block_header_json["txrt"]) !=
        TypeConverter::encodeBase64(merkle_tree_nodes.back())) {
      CLOG(ERROR, "BVAL") << "Invalid Merkle-tree root";
      return false;
    }

    auto block_time = static_cast<timestamp_type>(
        stoll(Safe::getString(block_header_json["time"])));

    BytesBuilder ssig_msg_wo_sid_builder;
    ssig_msg_wo_sid_builder.append(block_time);
    ssig_msg_wo_sid_builder.appendB64(
        Safe::getString(block_header_json["mID"]));
    ssig_msg_wo_sid_builder.appendB64(
        Safe::getString(block_header_json["cID"]));
    ssig_msg_wo_sid_builder.appendDec(
        Safe::getString(block_header_json["hgt"]));
    ssig_msg_wo_sid_builder.appendB64(
        Safe::getString(block_header_json["txrt"]));
    bytes ssig_msg_wo_sid = ssig_msg_wo_sid_builder.getBytes();

    auto storage_manager = Storage::getInstance();

    if (!block_header_json["SSig"].is_array()) {
      CLOG(ERROR, "BVAL") << "Invalid support signatures";
      return false;
    }

    for (size_t k = 0; k < block_header_json["SSig"].size(); ++k) {
      BytesBuilder ssig_msg_builder;
      ssig_msg_builder.appendB64(
          Safe::getString(block_header_json["SSig"][k]["sID"]));
      ssig_msg_builder.append(ssig_msg_wo_sid);
      BytesBuilder ssig_sig_builder;
      ssig_sig_builder.appendB64(
          Safe::getString(block_header_json["SSig"][k]["sig"]));
      std::string user_pk_pem;

      auto it_map = user_cert_map.find(
          Safe::getString(block_header_json["SSig"][k]["sID"]));
      if (it_map != user_cert_map.end()) {
        user_pk_pem = it_map->second;
      } else {
        user_pk_pem = storage_manager->findCertificate(
            Safe::getString(block_header_json["SSig"][k]["sID"]), block_time);
      }

      if (user_pk_pem.empty()) {
        CLOG(ERROR, "BVAL") << "No suitable user certificate";
        return false;
      }

      if (!RSA::doVerify(user_pk_pem, ssig_msg_builder.getString(),
                         ssig_sig_builder.getBytes(), true)) {
        return false;
      }
    }

    return true;
  }
};
} // namespace gruut