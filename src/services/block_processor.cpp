#include "block_processor.hpp"
#include <algorithm>
#include <cstring>
#include <queue>

namespace gruut {

bool BlockProcessor::messageProcess(InputMsgEntry &entry) {

  m_block_pool.emplace_back(entry);

  if (entry.type == (uint8_t)MessageType::MSG_REQ_BLOCK) {
    // TODO : storage read
    // m_storage.readBlock(entry.body["hgt"]);
    // TODO : make MSG_BLOCK

    // TODO : push to outputqueue
    // m_output_queue->push();
  }
  // msg_block
  else if (entry.type == (uint8_t)MessageType::MSG_BLOCK) {

    Botan::secure_vector<uint8_t> block_raw_bin =
        entry.body["blockraw"].get<std::string>();
    Botan::secure_vector<uint8_t> block_raw =
        Botan::base64_decode(entry.body["blockraw"].get<std::string>());
    union ByteToInt {
      uint8_t b[4];
      uint32_t t;
    };

    ByteToInt len_parse;
    len_parse.b[0] = block_raw[1];
    len_parse.b[1] = block_raw[2];
    len_parse.b[2] = block_raw[3];
    len_parse.b[3] = block_raw[4];

    size_t header_end = len_parse.t;
    std::string block_header_comp(block_raw.begin() + 5,
                                  block_raw.begin() + header_end);

    std::string block_header_json;
    if (block_raw[0] == (uint8_t)CompressionAlgorithmType::LZ4) {
      Compressor::decompressData(block_header_comp, block_header_json,
                                 (int)header_end - 5);
    } else if (block_raw[0] == (uint8_t)CompressionAlgorithmType::NONE) {
      block_header_json.assign(block_header_comp);
    } else {
      std::cout << "unknown compress type" << std::endl;
      return false;
    }

    // TODO : block 유효성 검사 - 서명 확인은 MessageValidator가 하기를 희망
    //  tx- rSig 확인, tx-digest, merkle_tree, ssig

    nlohmann::json json_header = nlohmann::json::parse(block_header_json);
    // json_header["txrt"] -> merkle tree root

    std::string test_pk_pem = R"UPK(-----BEGIN CERTIFICATE-----
MIIDLDCCAhQCBgEZlK1CPjANBgkqhkiG9w0BAQsFADBVMQswCQYDVQQGEwJBVTET
MBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQ
dHkgTHRkMQ4wDAYDVQQDDAVHcnV1dDAeFw0xODExMjQxNDEyMTRaFw0xODEyMjQx
NDEyMTRaMF4xCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYD
VQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQxFzAVBgNVBAMMDjEyMDM4MTAy
MzgwMTIzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA35Jf7Am6eBdy
zg5cmYHr+/tLvgKh8rIK0C9kJBFZ8a/se7XsDWjaF1Fxbm4YCCrY7pYAglBzOtJX
at1mi6TNgE9UGdyvo++R4sE2JSfCErCLEvtxPVV0f09LjOm2Z46Uc3AVXSdTVCas
OJxM3dda20/LlZT0xm7BtBpY7IspU/ZcqN4d2vaNbaZyCIQtzZV403eM6l92AhsA
cusOwlNLdw+7p/RlzjYs99vKyxLhz9mRPvsbnjJIurkRSjYX+C4jjNDvEJMOCCCH
UM2xy8dyYFpJFqqgcdjk6frWBMGbYRTvX4LNG4b2QOy/SAcvTOlQi/bKRLM+3XQG
hGXBXMn75QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCJs2hY8bMIWf4yw+5zbNIF
/aJqQRvu/FoeXkq8dcxxUj2c/s+rlxuoxhPUVnR3Q0fUwVxIN/23Ai3KFxk1nknO
7KkAsUkoCFJcZqYN2+rdIA/NJ6N1Hm8S7zXo5IexKmaNluMk3QoCBwraX+XgjGR6
FpgiTIvKlgMy97Mg/3rl3DyyC8MwsAF4Jpna16zYhkOKsOpB3/6zp10zvSNVaDhZ
dJ6MSWuZ1c6H/ConxqJJ4Ig274L9AYqV4KBslD9BN3+BSgPUOazCYkEkEgbNIBpr
IcLvsK86b2kKeMgNiI32t/M5rw53EzxKQyzg8vFqKtrj4z/UgtuK++G/PS575B9q
-----END CERTIFICATE-----)UPK";

    std::string test_sk_pem = R"USK(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDfkl/sCbp4F3LO
DlyZgev7+0u+AqHysgrQL2QkEVnxr+x7tewNaNoXUXFubhgIKtjulgCCUHM60ldq
3WaLpM2AT1QZ3K+j75HiwTYlJ8ISsIsS+3E9VXR/T0uM6bZnjpRzcBVdJ1NUJqw4
nEzd11rbT8uVlPTGbsG0GljsiylT9lyo3h3a9o1tpnIIhC3NlXjTd4zqX3YCGwBy
6w7CU0t3D7un9GXONiz328rLEuHP2ZE++xueMki6uRFKNhf4LiOM0O8Qkw4IIIdQ
zbHLx3JgWkkWqqBx2OTp+tYEwZthFO9fgs0bhvZA7L9IBy9M6VCL9spEsz7ddAaE
ZcFcyfvlAgMBAAECggEBAM+JzJuLiFrUwZEAiftCPPMkGvKe9QEbP6h0ZcyJguo1
uhw5C5CDJfkBdH/jmVFznP8VphFSZzVSby3Xqsq0yMN0YIjFcRKIYO+TFhU1rBW3
ZtLPMRaTjlpkHKkJh3boR2xFvr9Dsznp0HOYvE4vDLuLflwz82mFBTGQR74FjO7P
hoR4LIQ+Pk5MuyI7Q7Jn0DZ7hkNrnL1g5g+UCxGy/CDNPlGw48t9Ftpsy+z0gVan
Cgl3fAW0+ufHvm3QyxMlNmK2zuQKNcJSLeTTEtURV8ww5ZuKB9XEMOm2GzMCakRd
eeb1/wPqwyQEMHT9Ll01R2dIjU8u1ICI1x/R7rhL6mECgYEA86+LZnwGS18gY5Ck
6cqCvo38cA9F70CrU4FK/J3BboXME59i5fcUTSUA7l20cXi+hS2fAp81sKUc3nAu
tp0/B6dVeuzgVhfboRatM6BhGYHu+BjfwHAjPbgM7VBzJpVAwguuCO4ia8MWZZAk
Rd78QSbtKtgY+d8bZjxe6Kh/cz0CgYEA6t6f91fBq1mx0UnunduP+rTs/4fhGeIJ
b94P37C/36lkAsucYzZEOPV1YEWxw4OlN40BIco125KhWj6KE3HqrhGeaB0cuwqz
GuPFbO3MdDj51SUYrcGJuj2edXKZyCgsd6wFp5FWSdH8n8amQbk3aHmV1GUsys0V
BKWf0P+elckCgYAuuGhcpMi8KKfYDwJfRJFeoXBVt8frwBVY9EABQOm2G/btiDB4
8K82vzJ3gQW4f7Lfa8jBwu6TSITJbO632lwcRovP/pxgRUC5mNRqQoR7VHsRnAtC
JP3Mtn3b/gGl0xXQXlbmpWl6CbRAkqsxrjfk8eakwTvApHLnXgnAR5Xv7QKBgQCC
kUqahVWr/UwGDjSx2wp6lDQghhhUfD1EzE1EzIyOOSvZBfoliVh51bLv1y7QgxHJ
BQE5GKHCNAyxD41Q7AZLyI2oUW7UaElTTIZHXRdJEReKL3o9thbryy+ZGSF2jSbT
THVER16R4UOwSw3IAcBUuyrZDXnOMB5cG/rxg/lUSQKBgEtqijnRFo/CM4QpjXgH
okpmJkjwucI3JAGcdZ+tNgUxwKnmwhgkbS7NVuLkZlNqVHfRMzbzDIxprPFy9t5T
wCHB5M/bx9u2PbYfUTJq0m+qJyuWOCxbqi4JNRIKk9Lvy5EBBneU4wrCRovlh7Jg
Wcql+Gep9ebzfGArFp7anHE9
-----END PRIVATE KEY-----)USK";

    std::vector<std::vector<uint8_t>> tx_digests;

    if (entry.body["tx"].is_array()) {

      for (size_t i = 0; i < entry.body["tx"].size(); ++i) {
        std::vector<uint8_t> tx_id_raw = Botan::base64_decode(
            entry.body["tx"][i]["txid"].get<std::string>());
        int64_t time_64 = (int64_t)(entry.body["tx"][i]["time"].get<int64_t>());
        std::vector<uint8_t> rid_raw =
            Botan::base64_decode(entry.body["tx"][i]["rID"].get<std::string>());

        std::string msg;
        msg.resize(48);

        // std::memcpy(&msg[0], (const char*)tx_id_raw.data(),
        // tx_id_raw.size());
        std::memcpy(&msg[0], tx_id_raw.data(), tx_id_raw.size());
        std::memcpy(&msg[32], &time_64, sizeof(time_64));
        std::memcpy(&msg[40], rid_raw.data(), rid_raw.size());

        msg += entry.body["tx"][i]["type"];

        for (size_t j = 0; j < entry.body["tx"][i]["content"].size(); ++j) {
          msg += msg += entry.body["tx"][i]["content"][j];
        }

        std::string r_sig_b64 = entry.body["tx"][i]["rSig"].get<std::string>();
        std::vector<uint8_t> sig_raw(
            Botan::base64_encode_max_output(r_sig_b64.size()));
        size_t sig_raw_len = Botan::base64_decode(
            (uint8_t *)sig_raw.data(), (const char *)r_sig_b64.data(),
            r_sig_b64.size());

        std::vector<uint8_t> sig_raw_trim(sig_raw.begin(),
                                          sig_raw.begin() + sig_raw_len);

        if (RSA::doVerify(test_pk_pem, msg, sig_raw_trim, true)) {
          std::string digest_msg = msg;
          std::memcpy(&digest_msg[48], sig_raw_trim.data(),
                      sig_raw_trim.size());
          tx_digests.emplace_back(Sha256::hash(digest_msg));
        } else {
          std::cout << "invalid rSig" << std::endl;
          return false;
        }
      }
    } else {
      std::cout << "tx is not array" << std::endl;
      return false;
    }

    if (!tx_digests.empty()) {
      // merkle root check
      StaticMerkleTree merkle_tree;

      if (json_header["txrt"] == merkle_tree.generate(tx_digests).back()) {
        for (size_t k = 0; k < json_header["SSig"]["sID"].size(); ++k) {
          // signer 서명 확인
          std::string block_header_msg;
          block_header_msg.resize(72);

          Botan::secure_vector<uint8_t> signer_id_raw =
              Botan::base64_decode(json_header["SSig"][k]["sID"]);
          int64_t header_time = json_header["time"].get<int64_t>();
          Botan::secure_vector<uint8_t> mID_raw =
              Botan::base64_decode(json_header["mID"].get<std::string>());
          Botan::secure_vector<uint8_t> cID_raw =
              Botan::base64_decode(json_header["cID"].get<std::string>());
          std::string hgt = json_header["hgt"].get<std::string>();
          Botan::secure_vector<uint32_t> txrt_raw =
              Botan::base64_decode(json_header["txrt"].get<std::string>());

          std::memcpy(&block_header_msg[0], signer_id_raw.data(),
                      signer_id_raw.size());
          std::memcpy(&block_header_msg[8], &header_time, sizeof(header_time));
          std::memcpy(&block_header_msg[16], mID_raw.data(), mID_raw.size());
          std::memcpy(&block_header_msg[24], cID_raw.data(), cID_raw.size());
          std::memcpy(&block_header_msg[32], &hgt, hgt.size());
          std::memcpy(&block_header_msg[40], txrt_raw.data(), txrt_raw.size());

          Botan::secure_vector<uint8_t> signer_sig_raw =
              Botan::base64_decode(json_header["SSig"][k]["sig"]);

          RSA::doVerify(test_sk_pem, block_header_msg, signer_sig_raw, true);
        }
      } else {
        std::cout << "invalid merkle tree root" << std::endl;
      }
    } else {
      std::cout << "empty tx digests" << std::endl;
    }
    // TODO : storage save 추가
    Botan::secure_vector<uint8_t> block_tx =
        Botan::base64_decode(entry.body["tx"].get<std::string>());
    nlohmann::json block_tx_json = nlohmann::json::parse(block_tx);
    m_storage->saveBlock(block_raw_bin, block_header_json, block_tx_json);
  }
}

BlockProcessor::BlockProcessor() {
  m_input_queue = InputQueue::getInstance();
  m_output_queue = OutputQueue::getInstance();
  m_storage = Storage::getInstance();
}
} // namespace gruut