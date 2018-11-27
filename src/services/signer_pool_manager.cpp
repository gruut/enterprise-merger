#include <algorithm>
#include <botan/base64.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/x509cert.h>
#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

#include "../chain/types.hpp"
#include "../utils/random_number_generator.hpp"
#include "../utils/sha256.hpp"
#include "message_proxy.hpp"
#include "signer_pool_manager.hpp"

using namespace nlohmann;

namespace gruut {
const unsigned int REQUEST_NUM_OF_SIGNER = 5;

SignerPool SignerPoolManager::getSelectedSignerPool() {
  m_selected_signers_pool.reset();
  m_selected_signers_pool = std::make_shared<SignerPool>();

  const auto requested_signers_size = min(
      static_cast<unsigned int>(m_signer_pool->size()), REQUEST_NUM_OF_SIGNER);
  auto chosen_signers_index_set = generateRandomNumbers(requested_signers_size);

  for (auto index : chosen_signers_index_set)
    m_selected_signers_pool->insert(m_signer_pool->get(index));

  return *m_selected_signers_pool;
}

void SignerPoolManager::putSigner(Signer &&s) {
  m_signer_pool->insert(std::move(s));
}

void SignerPoolManager::handleMessage(MessageType &message_type,
                                      uint64_t receiver_id,
                                      json message_body_json) {
  MessageProxy proxy;
  vector<uint64_t> receiver_list{receiver_id};

  switch (message_type) {
  case MessageType::MSG_JOIN: {
    if (m_signer_pool->isFull()) {
      OutputMessage output_message =
          make_tuple(MessageType::MSG_ERROR, receiver_list, json({}));
      proxy.deliverOutputMessage(output_message);
    } else {
      json message_body;
      // TODO: Merger id 가 아직 결정 안되어서 임시값 할당
      sender_id_type sender_id = Sha256::hash("1");
      time_t now = time(nullptr);

      message_body["sender"] = Sha256::toString(sender_id);
      message_body["time"] = ctime(&now);

      m_merger_nonce =
          RandomNumGenerator::toString(RandomNumGenerator::randomize(64));
      message_body["mN"] = m_merger_nonce;

      OutputMessage output_message =
          make_tuple(MessageType::MSG_CHALLENGE, receiver_list, message_body);
      proxy.deliverOutputMessage(output_message);
    }
  } break;
  case MessageType::MSG_RESPONSE_1: {
    OutputMessage output_message;
    if (validateSignature(message_body_json)) {
      std::cout << "Validation success!" << std::endl;
      output_message =
          make_tuple(MessageType::MSG_RESPONSE_2, receiver_list, json({}));
    } else {
      output_message =
          make_tuple(MessageType::MSG_ERROR, receiver_list, json({}));
    }
    //    message_body[]
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

RandomSignerIndices
SignerPoolManager::generateRandomNumbers(unsigned int size) {
  // Generate random number in range(0, size)
  mt19937 mt;
  mt.seed(random_device()());

  RandomSignerIndices number_set;
  while (number_set.size() < size) {
    uniform_int_distribution<mt19937::result_type> dist(0, size - 1);
    int random_number = static_cast<int>(dist(mt));
    number_set.insert(random_number);
  }

  return number_set;
}

bool SignerPoolManager::validateSignature(json message_body_json) {
  auto signer_signature =
      Botan::base64_decode(message_body_json["sig"].get<string>());

  auto cert_vector =
      Botan::base64_decode(message_body_json["cert"].get<string>());
  vector<uint8_t> cert_in(cert_vector.begin(), cert_vector.end());

  Botan::X509_Certificate certificate(cert_in);
  Botan::RSA_PublicKey public_key(certificate.subject_public_key_algo(),
                                  certificate.subject_public_key_bitstring());
  auto tmp = certificate.subject_public_key_bitstring();

  Botan::PK_Verifier verifier(public_key, "EMSA3(SHA-256)");
  string target_signature = m_merger_nonce +
                            message_body_json["sN"].get<string>() +
                            message_body_json["dhx"].get<string>() +
                            message_body_json["dhy"].get<string>() +
                            message_body_json["time"].get<string>();
  verifier.update(target_signature);

  return verifier.check_signature(signer_signature);
}
} // namespace gruut