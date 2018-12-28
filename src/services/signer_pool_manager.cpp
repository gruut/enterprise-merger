
#include <algorithm>
#include <chrono>
#include <iostream>

#include <botan-2/botan/pem.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

#include "nlohmann/json.hpp"

#include "../utils/rsa.hpp"

#include "../application.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/hmac_key_maker.hpp"
#include "../utils/random_number_generator.hpp"
#include "../utils/sha256.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

#include "signer_pool_manager.hpp"

#include "easy_logging.hpp"

using namespace nlohmann;

namespace gruut {
SignerPoolManager::SignerPoolManager() {
  auto setting = Setting::getInstance();
  m_my_cert = setting->getMyCert();
  m_my_id = setting->getMyId();
  el::Loggers::getLogger("SMGR");
}
void SignerPoolManager::handleMessage(MessageType &message_type,
                                      json &message_body_json) {

  string recv_id_b64 = message_body_json["sID"].get<string>();
  signer_id_type recv_id =
      TypeConverter::decodeBase64(message_body_json["sID"].get<string>());

  vector<signer_id_type> receiver_list{recv_id};

  switch (message_type) {
  case MessageType::MSG_JOIN: {
    if (!isJoinable()) {
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_MAX_SIGNER_POOL);
      return;
    }

    auto current_time = Time::now_int();

    m_join_temp_table[recv_id_b64].reset(new JoinTemporaryData());

    m_join_temp_table[recv_id_b64]->start_time =
        static_cast<timestamp_type>(current_time);
    m_join_temp_table[recv_id_b64]->merger_nonce =
        PRNG::toString(PRNG::randomize(32));

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_CHALLENGE;
    output_message.body["mID"] = TypeConverter::encodeBase64(m_my_id);
    output_message.body["time"] = to_string(current_time);
    output_message.body["mN"] = m_join_temp_table[recv_id_b64]->merger_nonce;
    output_message.receivers = receiver_list;

    m_proxy.deliverOutputMessage(output_message);

  } break;
  case MessageType::MSG_RESPONSE_1: {
    if (m_join_temp_table.find(recv_id_b64) == m_join_temp_table.end()) {
      CLOG(ERROR, "SMGR") << "Illegal Trial";
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_ILLEGAL_ACCESS);
      return;
    }

    if (isTimeout(recv_id_b64)) {
      CLOG(ERROR, "SMGR") << "Join timeout (" << recv_id_b64 << ")";
      m_join_temp_table[recv_id_b64].release();
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_TIMEOUT,
                       "too late MSG_RESPONSE_1");
      return;
    }

    if (!verifySignature(recv_id, message_body_json)) {
      CLOG(ERROR, "SMGR") << "Invalid Signature";
      m_join_temp_table[recv_id_b64].release();
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_INVALID_SIG);
      return;
    }

    m_join_temp_table[recv_id_b64]->signer_cert =
        message_body_json["cert"].get<string>();

    json message_body;

    HmacKeyMaker key_maker;
    key_maker.genRandomSecretKey();
    auto public_key = key_maker.getPublicKey();

    string dhx = public_key.first;
    string dhy = public_key.second;

    auto signer_dhx = message_body_json["dhx"].get<string>();
    auto signer_dhy = message_body_json["dhy"].get<string>();

    auto shared_sk_bytes =
        key_maker.getSharedSecretKey(signer_dhx, signer_dhy, 32);

    if (shared_sk_bytes.empty()) {
      CLOG(ERROR, "SMGR") << "Failed to generate SSK (invalid PK)";
      m_join_temp_table[recv_id_b64].release();
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_INVALID_PK, "");
      return;
    }

    m_join_temp_table[recv_id_b64]->shared_secret_key =
        vector<uint8_t>(shared_sk_bytes.begin(), shared_sk_bytes.end());

    timestamp_type current_time = static_cast<timestamp_type>(Time::now_int());

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_RESPONSE_2;
    output_message.body["mID"] = TypeConverter::encodeBase64(m_my_id);
    output_message.body["time"] = to_string(current_time);
    output_message.body["cert"] = m_my_cert;
    output_message.body["dhx"] = dhx;
    output_message.body["dhy"] = dhy;
    output_message.body["sig"] = signMessage(
        m_join_temp_table[recv_id_b64]->merger_nonce,
        message_body_json["sN"].get<string>(), dhx, dhy, current_time);
    output_message.receivers = receiver_list;

    m_proxy.deliverOutputMessage(output_message);

    auto &signer_pool = Application::app().getSignerPool();
    auto secret_key_vector = TypeConverter::toSecureVector(
        m_join_temp_table[recv_id_b64]->shared_secret_key);
    signer_pool.pushSigner(recv_id, m_join_temp_table[recv_id_b64]->signer_cert,
                           secret_key_vector, SignerStatus::TEMPORARY);

  } break;
  case MessageType::MSG_SUCCESS: {
    // OK! This signer has passed HMAC on MesssageHandler.
    // If the merger is ok, it does not need check m_join_temp_table.

    if (isTimeout(recv_id_b64)) {
      CLOG(ERROR, "SMGR") << "Join timeout (" << recv_id_b64 << ")";
      m_join_temp_table[recv_id_b64].release();
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_TIMEOUT,
                       "too late MSG_SUCCESS");
      return;
    }

    m_join_temp_table[recv_id_b64].release();

    auto &signer_pool = Application::app().getSignerPool();
    signer_pool.updateStatus(recv_id, SignerStatus::GOOD);

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_ACCEPT;
    output_message.body["mID"] = TypeConverter::encodeBase64(m_my_id);
    output_message.body["time"] = Time::now();
    output_message.body["val"] = true;
    output_message.receivers = receiver_list;

    m_proxy.deliverOutputMessage(output_message);

  } break;
  case MessageType::MSG_LEAVE: {
    auto &signer_pool = Application::app().getSignerPool();
    if (m_join_temp_table.find(recv_id_b64) != m_join_temp_table.end())
      m_join_temp_table[recv_id_b64].release();

    if (signer_pool.removeSigner(recv_id)) {
      std::string leave_time = message_body_json["time"].get<string>();
      std::string leave_msg = message_body_json["msg"].get<string>();
      CLOG(INFO, "SMGR") << "Signer left (" << recv_id_b64 << ")";
    }

  } break;
  case MessageType::MSG_ECHO:
    break;
  default:
    break;
  }
}

bool SignerPoolManager::verifySignature(signer_id_type &signer_id,
                                        json &message_body_json) {

  bytes sig_bytes =
      TypeConverter::decodeBase64(message_body_json["sig"].get<string>());

  string cert_in = message_body_json["cert"].get<string>();

  string signer_id_b64 = TypeConverter::encodeBase64(signer_id);

  BytesBuilder msg_builder;
  msg_builder.appendB64(m_join_temp_table[signer_id_b64]->merger_nonce);
  msg_builder.appendB64(message_body_json["sN"].get<string>());
  msg_builder.appendHex(message_body_json["dhx"].get<string>());
  msg_builder.appendHex(message_body_json["dhy"].get<string>());
  msg_builder.appendDec(message_body_json["time"].get<string>());

  return RSA::doVerify(cert_in, msg_builder.getBytes(), sig_bytes, true);
}

string SignerPoolManager::signMessage(string merger_nonce, string signer_nonce,
                                      string dhx, string dhy,
                                      timestamp_type timestamp) {

  auto setting = Setting::getInstance();
  string rsa_sk_pem = setting->getMySK();
  string rsa_sk_pass = setting->getMyPass();

  BytesBuilder msg_builder;
  msg_builder.appendB64(merger_nonce);
  msg_builder.appendB64(signer_nonce);
  msg_builder.appendHex(dhx);
  msg_builder.appendHex(dhy);
  msg_builder.append(timestamp);

  return TypeConverter::encodeBase64(
      RSA::doSign(rsa_sk_pem, msg_builder.getBytes(), true, rsa_sk_pass));
}

bool SignerPoolManager::isJoinable() {
  return (config::MAX_SIGNER_NUM > Application::app().getSignerPool().size());
}

bool SignerPoolManager::isTimeout(std::string &signer_id_b64) {
  // Dont't call this function unless checking m_join_temp_table
  return (static_cast<timestamp_type>(Time::now_int()) -
              m_join_temp_table[signer_id_b64]->start_time >
          config::JOIN_TIMEOUT_SEC);
}

void SignerPoolManager::sendErrorMessage(vector<signer_id_type> &receiver_list,
                                         ErrorMsgType error_type,
                                         const std::string &info) {

  OutputMsgEntry output_message;
  output_message.type = MessageType::MSG_ERROR;
  output_message.body["sender"] = TypeConverter::encodeBase64(m_my_id); // my_id
  output_message.body["time"] = Time::now();
  output_message.body["type"] = std::to_string(static_cast<int>(error_type));
  output_message.body["info"] = info;
  output_message.receivers = receiver_list;

  m_proxy.deliverOutputMessage(output_message);
}
} // namespace gruut
