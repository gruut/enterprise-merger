#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include "nlohmann/json.hpp"

#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/ecdsa.hpp"
#include "../utils/hmac_key_maker.hpp"
#include "../utils/random_number_generator.hpp"
#include "../utils/sha256.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

#include "message_proxy.hpp"
#include "signer_pool.hpp"

#include <botan-2/botan/pem.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;

namespace gruut {
struct JoinTemporaryData {
  string merger_nonce;
  string signer_cert;
  vector<uint8_t> shared_secret_key;
  timestamp_t start_time;
  atomic<bool> join_lock;
};

class SignerPoolManager {
public:
  SignerPoolManager();
  void handleMessage(InputMsgEntry &input_message);

private:
  bool verifySignature(signer_id_type &signer_id, json &message_body_json);
  string signMessage(string, string, string, string, uint64_t);
  void sendErrorMessage(signer_id_type &recv_id, ErrorMsgType error_type,
                        const std::string &info = "");
  bool isJoinable();

  bool isTimeout(std::string &signer_id_b64);

  // A temporary table for connection establishment.
  unordered_map<string, unique_ptr<JoinTemporaryData>> m_join_temp_table;

  SignerPool *m_signer_pool;
  string m_my_cert;
  merger_id_type m_my_id;
  MessageProxy m_proxy;
};
} // namespace gruut
#endif
