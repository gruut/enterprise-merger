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
  static nlohmann::json getBlockJson(bytes &block_raw) {

    el::Loggers::getLogger("BVAL");

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

    el::Loggers::getLogger("BVAL");

    auto setting = Setting::getInstance();
    std::vector<MergerInfo> mergers = setting->getMergerInfo();

    std::vector<sha256> tx_digests;

    if (txs.empty() || !txs.is_array()) {
      CLOG(ERROR, "BVAL") << "TX is not array";
      return false;
    }

    tx_ids.clear();

    std::map<std::string, std::string> user_cert_map;

    for (auto &tx_one : txs) {
      BytesBuilder tx_digest_builder;
      tx_digest_builder.appendB64(Safe::getString(tx_one["txid"]));
      tx_digest_builder.appendDec(Safe::getString(tx_one["time"]));
      tx_digest_builder.appendB64(Safe::getString(tx_one["rID"]));
      tx_digest_builder.append(Safe::getString(tx_one["type"]));

      if(tx_one["content"].is_array()) {
        for (auto &each_content : tx_one["content"]) {
          tx_digest_builder.append(Safe::getString(each_content));
        }
      }

      if (Safe::getString(tx_one["type"]) == TXTYPE_CERTIFICATES) {
        if (tx_one["content"].is_array()){
          for (size_t j = 0; j < tx_one["content"].size(); j += 2) {
            user_cert_map[tx_one["content"][j]] = tx_one["content"][j + 1];
          }
        }
      } else { // except certificates
        std::string this_tx_id_b64 = Safe::getString(tx_one["txid"]);
        transaction_id_type this_tx_id =
            TypeConverter::base64ToArray<TRANSACTION_ID_TYPE_SIZE>(
                this_tx_id_b64);
        tx_ids.emplace_back(this_tx_id);
      }

      std::string rsig_b64 = Safe::getString(tx_one["rSig"]);
      bytes rsig_byte = TypeConverter::decodeBase64(rsig_b64);

      std::string cert =
          getMergerCert(mergers, Safe::getString(tx_one["rID"]));

      if (cert.empty()) {
        CLOG(ERROR, "BVAL") << "No certificate for sender";
        return false;
      }

      if (!RSA::doVerify(cert, tx_digest_builder.getString(), rsig_byte,
                         true)) {
        CLOG(ERROR, "BVAL") << "Invalid rSig";
        return false;
      }
      tx_digest_builder.appendB64(Safe::getString(tx_one["rSig"]));
      tx_digests.emplace_back(Sha256::hash(tx_digest_builder.getString()));
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    mtree_nodes = merkle_tree.getMerkleTree();

    BytesBuilder txrt_builder;
    txrt_builder.appendB64(Safe::getString(block_json["txrt"]));

    if (txrt_builder.getBytes() != mtree_nodes.back()) {
      CLOG(ERROR, "BVAL") << "Invalid Merkle-tree root";
      return false;
    }

    auto block_time = static_cast<timestamp_type>(
        stoll(Safe::getString(block_json["time"])));

    BytesBuilder ssig_msg_wo_sid_builder;
    ssig_msg_wo_sid_builder.append(block_time);
    ssig_msg_wo_sid_builder.appendB64(Safe::getString(block_json["mID"]));
    ssig_msg_wo_sid_builder.appendB64(Safe::getString(block_json["cID"]));
    ssig_msg_wo_sid_builder.appendDec(Safe::getString(block_json["hgt"]));
    ssig_msg_wo_sid_builder.appendB64(Safe::getString(block_json["txrt"]));
    bytes ssig_msg_wo_sid = ssig_msg_wo_sid_builder.getBytes();

    auto storage_manager = Storage::getInstance();

    if(!block_json["SSig"]["sID"].is_array()) {
      CLOG(ERROR, "BVAL") << "Invalid support signatures";
      return false;
    }

    for (size_t k = 0; k < block_json["SSig"]["sID"].size(); ++k) {
      BytesBuilder ssig_msg_builder;
      ssig_msg_builder.appendB64(Safe::getString(block_json["SSig"][k]["sID"]));
      ssig_msg_builder.append(ssig_msg_wo_sid);
      BytesBuilder ssig_sig_builder;
      ssig_sig_builder.appendB64(Safe::getString(block_json["SSig"][k]["sig"]));
      std::string user_pk_pem;

      auto it_map =
        user_cert_map.find(Safe::getString(block_json["SSig"][k]["sID"]));
      if (it_map != user_cert_map.end()) {
        user_pk_pem = it_map->second;
      } else {
        user_pk_pem = storage_manager->findCertificate(Safe::getString(block_json["SSig"][k]["sID"]), block_time);
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