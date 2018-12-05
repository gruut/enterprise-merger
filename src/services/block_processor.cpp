#include "block_processor.hpp"

namespace gruut {

bool BlockProcessor::messageProcess(InputMsgEntry &entry) {
  if (entry.type == MessageType::MSG_REQ_BLOCK) {
    auto msg_blk = m_storage->readBlock(entry.body["hgt"]);
    // TODO : make MSG_BLOCK

    OutputMsgEntry msg_block;
    msg_block.type = MessageType::MSG_BLOCK;
    // TODO : push to outputqueue
    // m_output_queue->push();
  } else if (entry.type == MessageType::MSG_BLOCK) {
    BytesBuilder block_raw_builder;
    block_raw_builder.appendB64(entry.body["blockraw"].get<std::string>());
    std::vector<uint8_t> block_raw = block_raw_builder.getBytes();

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
    if (block_raw[0] == (uint8_t)CompressionAlgorithmType::LZ4) {
      Compressor::decompressData(block_header_comp, block_json_str,
                                 (int)header_end - 5);
    } else if (block_raw[0] == (uint8_t)CompressionAlgorithmType::NONE) {
      block_json_str.assign(block_header_comp);
    } else {
      std::cout << "unknown compress type" << std::endl;
      return false;
    }

    nlohmann::json block_json = nlohmann::json::parse(block_json_str);

    std::vector<std::vector<uint8_t>> tx_digests;

    if (!entry.body["tx"].is_array() || entry.body["tx"].empty() == 0) {
      std::cout << "tx is not array" << std::endl;
      return false;
    }

    std::unordered_map<std::string, std::string> user_cert_map;

    for (size_t i = 0; i < entry.body["tx"].size(); ++i) {

      BytesBuilder tx_digest_builder;
      tx_digest_builder.appendB64(
          entry.body["tx"][i]["txid"].get<std::string>());
      tx_digest_builder.append(entry.body["tx"][i]["time"].get<int64_t>());
      tx_digest_builder.appendB64(
          entry.body["tx"][i]["rID"].get<std::string>());
      tx_digest_builder.append(entry.body["tx"][i]["type"].get<std::string>());

      for (size_t j : entry.body["tx"][i]["content"]) {
        tx_digest_builder.append(
            entry.body["tx"][i]["content"][j].get<std::string>());
      }

      if (entry.body["tx"][i]["type"].get<std::string>() == "certificates") {
        for (size_t j = 0; j < entry.body["tx"][i]["content"].size(); j += 2) {
          user_cert_map[entry.body["tx"][i]["content"][j]] =
              entry.body["tx"][i]["content"][j + 1];
        }
      }

      BytesBuilder rsig_builder;
      rsig_builder.appendB64(entry.body["tx"][i]["rSig"].get<std::string>());

      auto it_merger_cert =
          KNOWN_CERT_MAP.find(entry.body["tx"][i]["rID"].get<std::string>());

      if (it_merger_cert == KNOWN_CERT_MAP.end()) {
        std::cout << "no certificate for sender" << std::endl;
        return false;
      }

      if (!RSA::doVerify(it_merger_cert->second, tx_digest_builder.getString(),
                         rsig_builder.getBytes(), true)) {
        std::cout << "invalid rSig" << std::endl;
        return false;
      }

      tx_digest_builder.appendB64(
          entry.body["tx"][i]["rSig"].get<std::string>());
      tx_digests.emplace_back(Sha256::hash(tx_digest_builder.getString()));
    }

    MerkleTree merkle_tree;
    merkle_tree.generate(tx_digests);
    std::vector<sha256> mtree_nodes = merkle_tree.getMerkleTree();

    BytesBuilder txrt_builder;
    txrt_builder.appendB64(block_json["txrt"].get<std::string>());

    if (txrt_builder.getBytes() != mtree_nodes.back()) {
      std::cout << "invalid merkle tree root" << std::endl;
      return false;
    }

    BytesBuilder ssig_msg_wo_sid_builder;
    ssig_msg_wo_sid_builder.append(block_json["time"].get<int64_t>());
    ssig_msg_wo_sid_builder.appendB64(block_json["mID"].get<std::string>());
    ssig_msg_wo_sid_builder.appendB64(block_json["cID"].get<std::string>());
    ssig_msg_wo_sid_builder.appendDec(block_json["hgt"].get<std::string>());
    ssig_msg_wo_sid_builder.appendB64(block_json["txrt"].get<std::string>());
    std::vector<uint8_t> ssig_msg_wo_sid = ssig_msg_wo_sid_builder.getBytes();

    for (size_t k = 0; k < block_json["SSig"]["sID"].size(); ++k) {

      BytesBuilder ssig_msg_builder;
      ssig_msg_builder.appendB64(
          block_json["SSig"][k]["sID"].get<std::string>());
      ssig_msg_builder.append(ssig_msg_wo_sid);

      BytesBuilder ssig_sig_builder;
      ssig_sig_builder.appendB64(
          block_json["SSig"][k]["sig"].get<std::string>());

      std::string user_pk_pem;

      if (user_cert_map.empty()) {
        user_pk_pem = m_storage->findCertificate(
            block_json["SSig"][k]["sID"].get<std::string>());
      } else {
        auto it_map =
            user_cert_map.find(block_json["SSig"][k]["sID"].get<std::string>());
        if (it_map != user_cert_map.end())
          user_pk_pem = it_map->second;
        else
          user_pk_pem = m_storage->findCertificate(
              block_json["SSig"][k]["sID"].get<std::string>());
      }

      if (user_pk_pem.empty()) {
        return false;
      }

      if (!RSA::doVerify(user_pk_pem, ssig_msg_builder.getString(),
                         ssig_sig_builder.getBytes(), true)) {
        return false;
      }
    }

    nlohmann::json block_secret;
    block_secret["tx"] = entry.body["tx"];
    block_secret["txCnt"] = entry.body["tx"].size();

    nlohmann::json mtree(mtree_nodes.size(), "");
    for (size_t i = 0; i < mtree_nodes.size(); ++i) {
      mtree[i] = mtree_nodes[i];
    }

    block_secret["mtree"] = mtree;

    m_storage->saveBlock(entry.body["blockraw"].get<std::string>(), block_json,
                         block_secret);
  }
}

BlockProcessor::BlockProcessor() {
  m_input_queue = InputQueueAlt::getInstance();
  m_output_queue = OutputQueueAlt::getInstance();
  m_storage = Storage::getInstance();
}
} // namespace gruut