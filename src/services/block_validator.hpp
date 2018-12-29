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
  BlockValidator() { el::Loggers::getLogger("BVAL"); }

  static nlohmann::json getBlockJson(bytes &block_raw) {

    nlohmann::json block_json = {};

    if (block_raw.size() <= 5) {
      return block_json;
    }

    size_t header_end =
        static_cast<size_t>(block_raw[1] << 24 | block_raw[2] << 16 |
                            block_raw[3] << 8 | block_raw[4]);

    std::string block_header_comp(block_raw.begin() + 5,
                                  block_raw.begin() + header_end);
    std::string block_json_str;
    if (block_raw[0] == static_cast<uint8_t>(CompressionAlgorithmType::LZ4)) {
      block_json_str = Compressor::decompressData(block_header_comp);
    } else if (block_raw[0] ==
               static_cast<uint8_t>(CompressionAlgorithmType::NONE)) {
      block_json_str.assign(block_header_comp);
    } else {
      CLOG(ERROR, "BVAL") << "Unknown compress type";
      return block_json;
    }

    block_json = Safe::parseJson(block_json_str);
    if (block_json.empty())
      CLOG(ERROR, "BVAL") << "Invalid JSON structure";

    return block_json;
  }

  static std::string getMergerCert(std::vector<MergerInfo> mergers,
                                   const std::string &merger_id_b64) {
    merger_id_type merger_id = TypeConverter::decodeBase64(merger_id_b64);
    std::string merger_cert;
    if (!mergers.empty()) {
      for (auto &merger : mergers) {
        if (merger.id == merger_id) {
          merger_cert = merger.cert;
          break;
        }
      }
    }
    return merger_cert;
  }

  static bool validate(nlohmann::json &block_json, nlohmann::json &txs,
                       std::vector<sha256> &mtree_nodes,
                       std::vector<transaction_id_type> &tx_ids) {

    auto setting = Setting::getInstance();
    std::vector<MergerInfo> mergers = setting->getMergerInfo();

    std::vector<sha256> tx_digests;
    if (!txs.is_array() || txs.empty() == 0) {
      CLOG(ERROR, "BVAL") << "TX is not array";
      return false;
    }

    tx_ids.clear();

    std::map<std::string, std::string> user_cert_map;

    for (auto &tx_one : txs) {
      BytesBuilder tx_digest_builder;
      tx_digest_builder.appendB64(tx_one["txid"].get<std::string>());
      tx_digest_builder.appendDec(tx_one["time"].get<std::string>());
      tx_digest_builder.appendB64(tx_one["rID"].get<std::string>());
      tx_digest_builder.append(tx_one["type"].get<std::string>());

      for (auto &content_one : tx_one["content"]) {
        tx_digest_builder.append(content_one.get<std::string>());
      }

      if (tx_one["type"].get<std::string>() == TXTYPE_CERTIFICATES) {
        for (size_t j = 0; j < tx_one["content"].size(); j += 2) {
          user_cert_map[tx_one["content"][j]] = tx_one["content"][j + 1];
        }
      } else { // except certificates
        std::string this_tx_id_b64 = tx_one["txid"].get<std::string>();
        transaction_id_type this_tx_id =
            TypeConverter::base64ToArray<TRANSACTION_ID_TYPE_SIZE>(
                this_tx_id_b64);
        tx_ids.emplace_back(this_tx_id);
      }

      std::string rsig_b64 = tx_one["rSig"].get<std::string>();
      bytes rsig_byte = TypeConverter::decodeBase64(rsig_b64);

      std::string cert =
          getMergerCert(mergers, tx_one["rID"].get<std::string>());

      if (cert.empty()) {
        CLOG(ERROR, "BVAL") << "No certificate for sender";
        return false;
      }

      if (!RSA::doVerify(cert, tx_digest_builder.getString(), rsig_byte,
                         true)) {
        CLOG(ERROR, "BVAL") << "Invalid rSig";
        return false;
      }
      tx_digest_builder.appendB64(tx_one["rSig"].get<std::string>());
      tx_digests.emplace_back(Sha256::hash(tx_digest_builder.getString()));
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    mtree_nodes = merkle_tree.getMerkleTree();

    BytesBuilder txrt_builder;
    txrt_builder.appendB64(block_json["txrt"].get<std::string>());

    if (txrt_builder.getBytes() != mtree_nodes.back()) {
      CLOG(ERROR, "BVAL") << "Invalid Merkle-tree root";
      return false;
    }

    auto block_time = static_cast<timestamp_type>(
        stoll(block_json["time"].get<std::string>()));

    BytesBuilder ssig_msg_wo_sid_builder;
    ssig_msg_wo_sid_builder.append(block_time);
    ssig_msg_wo_sid_builder.appendB64(block_json["mID"].get<std::string>());
    ssig_msg_wo_sid_builder.appendB64(block_json["cID"].get<std::string>());
    ssig_msg_wo_sid_builder.appendDec(block_json["hgt"].get<std::string>());
    ssig_msg_wo_sid_builder.appendB64(block_json["txrt"].get<std::string>());
    bytes ssig_msg_wo_sid = ssig_msg_wo_sid_builder.getBytes();

    auto storage_manager = Storage::getInstance();

    for (size_t k = 0; k < block_json["SSig"]["sID"].size(); ++k) {
      BytesBuilder ssig_msg_builder;
      ssig_msg_builder.appendB64(
          block_json["SSig"][k]["sID"].get<std::string>());
      ssig_msg_builder.append(ssig_msg_wo_sid);
      BytesBuilder ssig_sig_builder;
      ssig_sig_builder.appendB64(
          block_json["SSig"][k]["sig"].get<std::string>());
      std::string user_pk_pem;

      auto it_map =
          user_cert_map.find(block_json["SSig"][k]["sID"].get<std::string>());
      if (it_map != user_cert_map.end()) {
        user_pk_pem = it_map->second;
      } else {
        user_pk_pem = storage_manager->findCertificate(
            block_json["SSig"][k]["sID"].get<std::string>(), block_time);
      }

      if (user_pk_pem.empty()) {
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