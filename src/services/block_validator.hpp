#pragma once

#include "../../include/nlohmann/json.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "setting.hpp"
#include "storage.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
namespace gruut {
class BlockValidator {

public:
  BlockValidator() {}

  static nlohmann::json getBlockJson(bytes &block_raw) {

    nlohmann::json block_json = {};

    union ByteToInt {
      uint8_t b[4];
      uint32_t t;
    };

    ByteToInt len_parse{};
    len_parse.b[0] = block_raw[1];
    len_parse.b[1] = block_raw[2];
    len_parse.b[2] = block_raw[3];
    len_parse.b[3] = block_raw[4];
    size_t header_end = len_parse.t;

    std::string block_header_comp(block_raw.begin() + 5,
                                  block_raw.begin() + header_end);
    std::string block_json_str;
    if (block_raw[0] == static_cast<uint8_t>(CompressionAlgorithmType::LZ4)) {
      Compressor::decompressData(block_header_comp, block_json_str,
                                 (int)header_end - 5);
    } else if (block_raw[0] ==
               static_cast<uint8_t>(CompressionAlgorithmType::NONE)) {
      block_json_str.assign(block_header_comp);
    } else {
      std::cout << "unknown compress type" << std::endl;
      return block_json;
    }

    try {
      block_json = nlohmann::json::parse(block_json_str);
    } catch (json::parse_error &e) {
      std::cout << "Received block contains invalid json structure : "
                << e.what() << endl;
    }

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
                       std::vector<sha256> &mtree_nodes) {

    auto setting = Setting::getInstance();
    std::vector<MergerInfo> mergers = setting->getMergerInfo();

    std::vector<sha256> tx_digests;
    if (!txs.is_array() || txs.empty() == 0) {
      std::cout << "tx is not array" << std::endl;
      return false;
    }

    std::map<std::string, std::string> user_cert_map;

    for (size_t i = 0; i < txs.size(); ++i) {
      BytesBuilder tx_digest_builder;
      tx_digest_builder.appendB64(txs[i]["txid"].get<std::string>());
      tx_digest_builder.appendDec(txs[i]["time"].get<std::string>());
      tx_digest_builder.appendB64(txs[i]["rID"].get<std::string>());
      tx_digest_builder.append(txs[i]["type"].get<std::string>());

      for (size_t j = 0; j < txs[i]["content"].size(); ++j) {
        tx_digest_builder.append(txs[i]["content"][j].get<std::string>());
      }

      if (txs[i]["type"].get<std::string>() == TXTYPE_CERTIFICATES) {
        for (size_t j = 0; j < txs[i]["content"].size(); j += 2) {
          user_cert_map[txs[i]["content"][j]] = txs[i]["content"][j + 1];
        }
      }

      std::string rsig_b64 = txs[i]["rSig"].get<std::string>();
      bytes rsig_byte = TypeConverter::decodeBase64(rsig_b64);

      std::string cert =
          getMergerCert(mergers, txs[i]["rID"].get<std::string>());

      if (cert.empty()) {
        std::cout << "no certificate for sender" << std::endl;
        return false;
      }

      if (!RSA::doVerify(cert, tx_digest_builder.getString(), rsig_byte,
                         true)) {
        std::cout << "invalid rSig" << std::endl;
        return false;
      }
      tx_digest_builder.appendB64(txs[i]["rSig"].get<std::string>());
      tx_digests.emplace_back(Sha256::hash(tx_digest_builder.getString()));
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    mtree_nodes = merkle_tree.getMerkleTree();

    BytesBuilder txrt_builder;
    txrt_builder.appendB64(block_json["txrt"].get<std::string>());

    if (txrt_builder.getBytes() != mtree_nodes.back()) {
      std::cout << "invalid merkle tree root" << std::endl;
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