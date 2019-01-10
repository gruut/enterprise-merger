#include "signer_pool_manager.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

namespace gruut {
SignerPoolManager::SignerPoolManager() {
  auto setting = Setting::getInstance();
  m_my_cert = setting->getMyCert();
  m_my_id = setting->getMyId();
  m_signer_pool = SignerPool::getInstance();
  el::Loggers::getLogger("SMGR");
}
void SignerPoolManager::handleMessage(MessageType &message_type,
                                      json &message_body_json) {

  string recv_id_b64 = Safe::getString(message_body_json, "sID");
  signer_id_type recv_id = Safe::getBytesFromB64(message_body_json, "sID");

  vector<signer_id_type> receiver_list{recv_id};

  switch (message_type) {
  case MessageType::MSG_JOIN: {
    if (!isJoinable()) {
      CLOG(ERROR, "SMGR") << "Merger is full";
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_MAX_SIGNER_POOL);
      return;
    }

    auto current_time = Time::now_int();

    m_join_temp_table[recv_id_b64].reset(new JoinTemporaryData());
    m_join_temp_table[recv_id_b64]->join_lock = true;
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
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_TIMEOUT,
                       "too late MSG_RESPONSE_1");
      return;
    }

    if (!verifySignature(recv_id, message_body_json)) {
      CLOG(ERROR, "SMGR") << "Invalid Signature";
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_INVALID_SIG);
      return;
    }

    m_join_temp_table[recv_id_b64]->signer_cert =
        Safe::getString(message_body_json, "cert");

    json message_body;

    HmacKeyMaker key_maker;
    key_maker.genRandomSecretKey();
    auto public_key = key_maker.getPublicKey();

    string dhx = public_key.first;
    string dhy = public_key.second;

    auto signer_dhx = Safe::getString(message_body_json, "dhx");
    auto signer_dhy = Safe::getString(message_body_json, "dhy");

    auto shared_sk_bytes =
        key_maker.getSharedSecretKey(signer_dhx, signer_dhy, 32);

    if (shared_sk_bytes.empty()) {
      CLOG(ERROR, "SMGR") << "Failed to generate SSK (invalid PK)";
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
        Safe::getString(message_body_json, "sN"), dhx, dhy, current_time);
    output_message.receivers = receiver_list;

    m_proxy.deliverOutputMessage(output_message);

    auto secret_key_vector = TypeConverter::toSecureVector(
        m_join_temp_table[recv_id_b64]->shared_secret_key);
    m_signer_pool->pushSigner(recv_id,
                              m_join_temp_table[recv_id_b64]->signer_cert,
                              secret_key_vector, SignerStatus::TEMPORARY);
    m_join_temp_table[recv_id_b64]->join_lock = false;
  } break;
  case MessageType::MSG_SUCCESS: {
    // OK! This signer has passed HMAC on MesssageHandler.
    // If the merger is ok, it does not need check m_join_temp_table.

    if (isTimeout(recv_id_b64)) {
      CLOG(ERROR, "SMGR") << "Join timeout (" << recv_id_b64 << ")";
      sendErrorMessage(receiver_list, ErrorMsgType::ECDH_TIMEOUT,
                       "too late MSG_SUCCESS");
      return;
    }

    m_signer_pool->updateStatus(recv_id, SignerStatus::GOOD);
    m_join_temp_table.erase(recv_id_b64);

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_ACCEPT;
    output_message.body["mID"] = TypeConverter::encodeBase64(m_my_id);
    output_message.body["time"] = Time::now();
    output_message.body["val"] = true;
    output_message.receivers = receiver_list;

    m_proxy.deliverOutputMessage(output_message);

  } break;
  case MessageType::MSG_LEAVE: {
    if (m_join_temp_table.find(recv_id_b64) != m_join_temp_table.end()) {
      if(!m_join_temp_table[recv_id_b64]->join_lock)
        m_join_temp_table.erase(recv_id_b64);
    }
    if (m_signer_pool->removeSigner(recv_id)) {
      std::string leave_time = Safe::getString(message_body_json, "time");
      std::string leave_msg = Safe::getString(message_body_json, "msg");
      CLOG(INFO, "SMGR") << "Signer left (" << recv_id_b64 << ")";
    }

  } break;
  default:
    break;
  }
}

bool SignerPoolManager::verifySignature(signer_id_type &signer_id,
                                        json &message_body_json) {

  bytes sig_bytes = Safe::getBytesFromB64(message_body_json, "sig");

  string cert_in = Safe::getString(message_body_json, "cert");

  string signer_id_b64 = TypeConverter::encodeBase64(signer_id);

  BytesBuilder msg_builder;
  msg_builder.appendB64(m_join_temp_table[signer_id_b64]->merger_nonce);
  msg_builder.appendB64(Safe::getString(message_body_json, "sN"));
  msg_builder.appendHex(Safe::getString(message_body_json, "dhx"));
  msg_builder.appendHex(Safe::getString(message_body_json, "dhy"));
  msg_builder.appendDec(Safe::getString(message_body_json, "time"));

  return ECDSA::doVerify(cert_in, msg_builder.getBytes(), sig_bytes);
}

string SignerPoolManager::signMessage(string merger_nonce, string signer_nonce,
                                      string dhx, string dhy,
                                      timestamp_type timestamp) {

  auto setting = Setting::getInstance();
  string ecdsa_sk_pem = setting->getMySK();
  string ecdsa_sk_pass = setting->getMyPass();

  BytesBuilder msg_builder;
  msg_builder.appendB64(merger_nonce);
  msg_builder.appendB64(signer_nonce);
  msg_builder.appendHex(dhx);
  msg_builder.appendHex(dhy);
  msg_builder.append(timestamp);

  return TypeConverter::encodeBase64(
      ECDSA::doSign(ecdsa_sk_pem, msg_builder.getBytes(), ecdsa_sk_pass));
}

bool SignerPoolManager::isJoinable() {
  return (config::MAX_SIGNER_NUM > m_signer_pool->size());
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
