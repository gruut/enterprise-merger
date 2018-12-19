
#include <algorithm>
#include <chrono>
#include <iostream>

#include <botan-2/botan/base64.h>
#include <botan-2/botan/pem.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

#include <nlohmann/json.hpp>

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

#include "message_proxy.hpp"
#include "signer_pool_manager.hpp"

using namespace nlohmann;

namespace gruut {
SignerPoolManager::SignerPoolManager() {
  Setting *setting = Setting::getInstance();
  m_my_cert = setting->getMyCert();
  m_my_id = setting->getMyId();
}
void SignerPoolManager::handleMessage(MessageType &message_type,
                                      json &message_body_json) {

  string recv_id_b64 = message_body_json["sender"].get<string>();
  signer_id_type recv_id = TypeConverter::decodeBase64(recv_id_b64);

  MessageProxy proxy;
  vector<signer_id_type> receiver_list{recv_id};

  auto current_time = Time::now_int();
  string timestamp = to_string(current_time);
  switch (message_type) {
  case MessageType::MSG_JOIN: {
    if (!isJoinable()) {
      deliverErrorMessage(receiver_list);
    } else {
      m_join_temporary_table[recv_id_b64].reset(new JoinTemporaryData());
      m_join_temporary_table[recv_id_b64]->expires_at =
          Time::from_now(config::JOIN_TIMEOUT_SEC);

      json message_body;
      message_body["sender"] = TypeConverter::toBase64Str(m_my_id);
      message_body["time"] = timestamp;

      m_join_temporary_table[recv_id_b64]->merger_nonce =
          RandomNumGenerator::toString(RandomNumGenerator::randomize(32));
      message_body["mN"] = m_join_temporary_table[recv_id_b64]->merger_nonce;

      OutputMsgEntry output_message;
      output_message.type = MessageType::MSG_CHALLENGE;
      output_message.body = message_body;
      output_message.receivers = receiver_list;

      proxy.deliverOutputMessage(output_message);
    }
  } break;
  case MessageType::MSG_RESPONSE_1: {
    if (isTimeout(recv_id_b64)) {
      m_join_temporary_table[recv_id_b64].release();
      deliverErrorMessage(receiver_list);
      return;
    }

    OutputMsgEntry output_message;
    if (verifySignature(recv_id, message_body_json)) {
      std::cout << "Validation success!" << std::endl;

      auto signer_pk_cert = message_body_json["cert"].get<string>();
      cout << " signer_pk_cert original is  " << signer_pk_cert << endl;
//      auto cert_vector = TypeConverter::decodeBase64(signer_pk_cert);
//      string decoded_cert_str(cert_vector.begin(), cert_vector.end());
      m_join_temporary_table[recv_id_b64]->signer_cert = signer_pk_cert;

      json message_body;
      message_body["sender"] = TypeConverter::toBase64Str(m_my_id);
      message_body["time"] = timestamp;
      message_body["cert"] = m_my_cert;

      HmacKeyMaker key_maker;
      key_maker.genRandomSecretKey();
      auto public_key = key_maker.getPublicKey();

      string dhx = public_key.first;
      string dhy = public_key.second;
      message_body["dhx"] = dhx;
      message_body["dhy"] = dhy;

      auto signer_dhx = message_body_json["dhx"].get<string>();
      auto signer_dhy = message_body_json["dhy"].get<string>();

      auto shared_secret_key_vector =
          key_maker.getSharedSecretKey(signer_dhx, signer_dhy, 32);
      m_join_temporary_table[recv_id_b64]->shared_secret_key = vector<uint8_t>(
          shared_secret_key_vector.begin(), shared_secret_key_vector.end());

      message_body["sig"] = signMessage(
          m_join_temporary_table[recv_id_b64]->merger_nonce,
          message_body_json["sN"].get<string>(), dhx, dhy, current_time);

      auto &signer_pool = Application::app().getSignerPool();
      auto secret_key_vector = TypeConverter::toSecureVector(
          m_join_temporary_table[recv_id_b64]->shared_secret_key);
      signer_pool.pushSigner(recv_id,
                             m_join_temporary_table[recv_id_b64]->signer_cert,
                             secret_key_vector, SignerStatus::TEMPORARY);

      output_message.type = MessageType::MSG_RESPONSE_2;
      output_message.body = message_body;
      output_message.receivers = receiver_list;

    } else {
      m_join_temporary_table[recv_id_b64].release();
      output_message.type = MessageType::MSG_ERROR;
      output_message.body = json({});
      output_message.receivers = receiver_list;
    }
    //    message_body[]
    proxy.deliverOutputMessage(output_message);
  } break;
  case MessageType::MSG_SUCCESS: {
    auto &signer_pool = Application::app().getSignerPool();

    if (isTimeout(recv_id_b64)) {
      m_join_temporary_table[recv_id_b64].release();
      signer_pool.removeSigner(recv_id);
      deliverErrorMessage(receiver_list);
      return;
    }

    signer_pool.updateStatus(recv_id, SignerStatus::GOOD);

    json message_body;
    message_body["sender"] = TypeConverter::toBase64Str(m_my_id);
    message_body["time"] = timestamp;
    message_body["val"] = true;

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_ACCEPT;
    output_message.body = message_body;
    output_message.receivers = receiver_list;

    proxy.deliverOutputMessage(output_message);
  } break;
  case MessageType::MSG_ECHO:
    break;
  case MessageType::MSG_LEAVE:
    break;
  default:
    break;
  }
}

bool SignerPoolManager::verifySignature(signer_id_type &signer_id,
                                        json &message_body_json) {
  const auto decoded_signer_signature =
      Botan::base64_decode(message_body_json["sig"].get<string>());
  const auto cert_vector =
      Botan::base64_decode(message_body_json["cert"].get<string>());

  const string cert_in(cert_vector.begin(), cert_vector.end());
  const vector<uint8_t> signer_signature(decoded_signer_signature.begin(),
                                         decoded_signer_signature.end());

  string signer_id_b64 = TypeConverter::toBase64Str(signer_id);

  BytesBuilder sig_builder;
  sig_builder.appendB64(m_join_temporary_table[signer_id_b64]->merger_nonce);
  sig_builder.appendB64(message_body_json["sN"].get<string>());
  sig_builder.appendHex(message_body_json["dhx"].get<string>());
  sig_builder.appendHex(message_body_json["dhy"].get<string>());
  sig_builder.appendDec(message_body_json["time"].get<string>());

  const bytes message_bytes = sig_builder.getBytes();

  return RSA::doVerify(cert_in, message_bytes, signer_signature, true);
}

string SignerPoolManager::signMessage(string merger_nonce, string signer_nonce,
                                      string dhx, string dhy,
                                      timestamp_type timestamp) {

  Setting *setting = Setting::getInstance();
  string rsa_sk_pem = setting->getMySK();
  string rsa_sk_pass = setting->getMyPass();

  BytesBuilder builder;
  builder.appendB64(merger_nonce);
  builder.appendB64(signer_nonce);
  builder.appendHex(dhx);
  builder.appendHex(dhy);
  builder.append(timestamp);

  auto message_bytes = builder.getBytes();
  auto signature = RSA::doSign(rsa_sk_pem, message_bytes, true, rsa_sk_pass);

  return Botan::base64_encode(signature);
}

bool SignerPoolManager::isJoinable() {
  return (config::MAX_SIGNER_NUM > Application::app().getSignerPool().size());
}

bool SignerPoolManager::isTimeout(string &signer_id_b64) {
  if (m_join_temporary_table[signer_id_b64]) {
    auto expires_at = m_join_temporary_table[signer_id_b64]->expires_at;
    auto timeout = expires_at < Time::now_int();
    // TODO: Logger
    if (timeout)
      std::cout << "Signer " << signer_id_b64 << " is expired" << std::endl;
    return timeout;
  }
  return false;
}

void SignerPoolManager::deliverErrorMessage(
    vector<signer_id_type> &receiver_list) {
  MessageProxy proxy;

  OutputMsgEntry output_message;
  output_message.type = MessageType::MSG_ERROR;
  output_message.body = json({});
  output_message.receivers = receiver_list;

  proxy.deliverOutputMessage(output_message);
}
} // namespace gruut