#include <algorithm>
#include <botan/base64.h>
#include <botan/pem.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/x509cert.h>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../utils/rsa.hpp"

#include "../application.hpp"
#include "../chain/types.hpp"
#include "../utils/hmac_key_maker.hpp"
#include "../utils/random_number_generator.hpp"
#include "../utils/sha256.hpp"
#include "../utils/type_converter.hpp"
#include "message_proxy.hpp"
#include "signer_pool_manager.hpp"

using namespace nlohmann;

namespace gruut {
void SignerPoolManager::handleMessage(MessageType &message_type,
                                      signer_id_type receiver_id,
                                      json message_body_json) {
  MessageProxy proxy;
  vector<uint64_t> receiver_list{receiver_id};

  auto now = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  string timestamp = to_string(now);
  switch (message_type) {
  case MessageType::MSG_JOIN: {
    if (!isJoinable()) {
      OutputMessage output_message =
          make_tuple(MessageType::MSG_ERROR, receiver_list, json({}));
      proxy.deliverOutputMessage(output_message);
    } else {
      join_temporary_table[receiver_id].reset(new JoinTemporaryData());

      json message_body;
      // TODO: Merger id 가 아직 결정 안되어서 임시값 할당
      sender_id_type sender_id = Sha256::hash("1");

      message_body["sender"] = Sha256::toString(sender_id);
      message_body["time"] = timestamp;

      join_temporary_table[receiver_id]->merger_nonce =
          RandomNumGenerator::toString(RandomNumGenerator::randomize(32));
      message_body["mN"] = join_temporary_table[receiver_id]->merger_nonce;

      OutputMessage output_message =
          make_tuple(MessageType::MSG_CHALLENGE, receiver_list, message_body);
      proxy.deliverOutputMessage(output_message);
    }
  } break;
  case MessageType::MSG_RESPONSE_1: {
    OutputMessage output_message;
    if (verifySignature(receiver_id, message_body_json)) {
      std::cout << "Validation success!" << std::endl;

      // TODO: 임시로 Merger ID 1로 함
      sender_id_type sender_id = Sha256::hash("1");

      auto signer_pk_cert = message_body_json["cert"].get<string>();
      auto cert_vector = Botan::base64_decode(signer_pk_cert);
      string decoded_cert_str(cert_vector.begin(), cert_vector.end());
      join_temporary_table[receiver_id]->signer_cert = decoded_cert_str;

      json message_body;
      message_body["sender"] = Sha256::toString(sender_id);
      message_body["time"] = timestamp;
      message_body["cert"] = getCertificate();

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
      join_temporary_table[receiver_id]->shared_secret_key = vector<uint8_t>(
          shared_secret_key_vector.begin(), shared_secret_key_vector.end());

      auto merger_nonce_bytes = TypeConverter::toBytes(
          join_temporary_table[receiver_id]->merger_nonce);
      auto signer_nonce_bytes =
          TypeConverter::toBytes(message_body_json["sN"].get<string>());
      auto dhx_bytes = TypeConverter::toBytes(dhx);
      auto dhy_bytes = TypeConverter::toBytes(dhy);
      auto timestamp_bytes = TypeConverter::toBytes(timestamp);

      message_body["sig"] = signMessage(merger_nonce_bytes, signer_nonce_bytes,
                                        dhx_bytes, dhy_bytes, timestamp_bytes);

      output_message =
          make_tuple(MessageType::MSG_RESPONSE_2, receiver_list, message_body);
    } else {
      output_message =
          make_tuple(MessageType::MSG_ERROR, receiver_list, json({}));
    }
    //    message_body[]
    proxy.deliverOutputMessage(output_message);
  } break;
  case MessageType::MSG_SUCCESS: {
    auto &signer_pool = Application::app().getSignerPool();

    // TODO: 임시로 Merger ID 1로 함
    sender_id_type merger_id = Sha256::hash("1");

    auto secret_key_vector = TypeConverter::toSecureVector(
        join_temporary_table[receiver_id]->shared_secret_key);
    signer_pool.pushSigner(receiver_id,
                           join_temporary_table[receiver_id]->signer_cert,
                           secret_key_vector, SignerStatus::GOOD);

    json message_body;
    message_body["sender"] = Sha256::toString(merger_id);
    message_body["time"] = timestamp;
    message_body["val"] = true;

    OutputMessage output_message;
    output_message =
        make_tuple(MessageType::MSG_ACCEPT, receiver_list, message_body);

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

bool SignerPoolManager::verifySignature(signer_id_type signer_id,
                                        json message_body_json) {
  const auto decoded_signer_signature =
      Botan::base64_decode(message_body_json["sig"].get<string>());
  const auto cert_vector =
      Botan::base64_decode(message_body_json["cert"].get<string>());

  const string cert_in(cert_vector.begin(), cert_vector.end());
  const vector<uint8_t> signer_signature(decoded_signer_signature.begin(),
                                         decoded_signer_signature.end());

  const string message = join_temporary_table[signer_id]->merger_nonce +
                         message_body_json["sN"].get<string>() +
                         message_body_json["dhx"].get<string>() +
                         message_body_json["dhy"].get<string>() +
                         message_body_json["time"].get<string>();

  return RSA::doVerify(cert_in, message, signer_signature, true);
}

string SignerPoolManager::getCertificate() {
  // TODO: config 파일 만들어서 하드코딩 된 것 처리할 것.
  const string tmp_cert =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIC2jCCAcKgAwIBAgIBATANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVW\n"
      "YXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVv\n"
      "bjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjkwMTI5\n"
      "MzhaFw0xOTExMjkwMTI5MzhaMAAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
      "AoIBAQCKWX3bNHseiGPQMSzoJt0kGmPlhp7BYj2LLPEexBl2RNFPyqhpmgROlY91\n"
      "GbQTUB9B5/wR+agn/bMF6jKtNH27HqWXeiJxtlDCknOjJZLbdhhwynyWxmzgHDG3\n"
      "4beuHK8rYQYzcXuOcGPKKP0impIzs8jQZQfJu64bU9GjY7ElVvQNBzOHODBpCzpv\n"
      "6AQ2UXfXt57T/vNAG6UMfuB+uTrW8q4d3raHsy7VPEUG3os9wteny5OZdIQMaeSi\n"
      "xtUMJjH0BeaTaEg3GuzxLV/YkZzCJ7HOXmU1DlXWCk/L0/w1sseKwIohS3WPXzsd\n"
      "TbU3zfQzvHKgCR3wkkUTS5csmTxpAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEOx\n"
      "14/mmitlmxmOl5PK5PJNtotbyZJ5KttMWtBV5jizElJ8JzBGEz4goiz50Y/KGVbP\n"
      "4tNUCs0XADaJrC7KmmxDUJZUU2iIkK27hqexpwFCsqAdQUAa7kvBe4C+cE5hqVIa\n"
      "1aMkoufdmUR98VNQkatKuWtrqJRtWIwMeUj0lO6rxnY4HSlv56/h6lJGJJAleiyF\n"
      "mE8QW35n2uPz//RDV0J3Rj5xzu6tmHNJp+vdibQeqIiESak2vlkRD/7I4mrx7hf4\n"
      "Ghu7BKUmzhIPAzZR/jp/AEzWeiLisnWZeoj7l6dSRrBBiCcMF+RccxAaWd4abJxg\n"
      "5PMxcZ8IUhvjaHsIO5U=\n"
      "-----END CERTIFICATE-----";

  return tmp_cert;
}

string SignerPoolManager::signMessage(vector<uint8_t> merger_nonce,
                                      vector<uint8_t> signer_nonce,
                                      vector<uint8_t> dhx, vector<uint8_t> dhy,
                                      vector<uint8_t> timestamp) {
  // TODO: 임시 rsa_sk_pem
  string rsa_sk_pem = "";

  vector<uint8_t> message_bytes;
  message_bytes.insert(message_bytes.end(), merger_nonce.begin(),
                       merger_nonce.end());
  message_bytes.insert(message_bytes.end(), signer_nonce.begin(),
                       signer_nonce.end());
  message_bytes.insert(message_bytes.end(), dhx.begin(), dhx.end());
  message_bytes.insert(message_bytes.end(), dhy.begin(), dhy.end());
  message_bytes.insert(message_bytes.end(), timestamp.begin(), timestamp.end());

  auto signature = RSA::doSign(rsa_sk_pem, message_bytes, true);

  return Botan::base64_encode(signature);
}

bool SignerPoolManager::isJoinable() {
  // TODO: 현재 100명 정도 가입할 수 있다. Config 관련 코드 구현하면 제거할 것
  return true;
}
} // namespace gruut