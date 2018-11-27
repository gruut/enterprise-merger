#include <algorithm>
#include <botan/base64.h>
#include <botan/pem.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/x509cert.h>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

#include "../chain/types.hpp"
#include "../utils/hmac_key_maker.hpp"
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

  auto now = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  string timestamp = to_string(now);
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

      message_body["sender"] = Sha256::toString(sender_id);
      message_body["time"] = timestamp;

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
    if (verifySignature(message_body_json)) {
      std::cout << "Validation success!" << std::endl;

      sender_id_type sender_id = Sha256::hash("1");

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

      string message = m_merger_nonce + message_body_json["sN"].get<string>() +
                       dhx + dhy + timestamp;
      message_body["sig"] = signMessage(message);

      output_message =
          make_tuple(MessageType::MSG_RESPONSE_2, receiver_list, message_body);
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

bool SignerPoolManager::verifySignature(json message_body_json) {
  auto signer_signature =
      Botan::base64_decode(message_body_json["sig"].get<string>());

  auto cert_vector =
      Botan::base64_decode(message_body_json["cert"].get<string>());
  vector<uint8_t> cert_in(cert_vector.begin(), cert_vector.end());

  Botan::X509_Certificate certificate(cert_in);
  Botan::RSA_PublicKey public_key(certificate.subject_public_key_algo(),
                                  certificate.subject_public_key_bitstring());

  Botan::PK_Verifier verifier(public_key, "EMSA3(SHA-256)");
  string target_signature = m_merger_nonce +
                            message_body_json["sN"].get<string>() +
                            message_body_json["dhx"].get<string>() +
                            message_body_json["dhy"].get<string>() +
                            message_body_json["time"].get<string>();
  verifier.update(target_signature);

  return verifier.check_signature(signer_signature);
}

string SignerPoolManager::getCertificate() {
  string tmp_cert =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIC2jCCAcKgAwIBAgIBATANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVW\n"
      "YXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVv\n"
      "bjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjgwNTA3\n"
      "MTVaFw0xOTExMjgwNTA3MTVaMAAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
      "AoIBAQCVRiRanR2WTfz42k+/XhRYqdZdAJOgY8w2Bbvn+ZTNrm/AVy/zjMn7q9oJ\n"
      "2y0ConhFn3aeJ8PxTYl2+p65jMZvjW0uxNAdhnnJAqx/Yu5dMBfhEnpihG633Nse\n"
      "14pgEquTlZZm8efazIVhJtMUXSMdB81mwnaWbvmFhxoddXHrB5CTQtcotf0VN8zG\n"
      "T1MSLHAIxAspl+JN0vJpBVC0YTmMDMnVGUBFEgYFMF2kRgEiSVoZqjiX9Qa0YCSn\n"
      "zO4VxJlXgwjsJX2wq+mV46jcRypqoC5jI+PV7oYpmlmHJHhmB9X5EOmkcul8zIkX\n"
      "0BZRrbdgkG1OUdJnxsrahv3ciApbAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAIJ5\n"
      "8+Ob1N0qn6go0USjPn83ZBOYU/aigl5TJas4egbDyb91Ndmq7xLyB/wbp1ZGAm89\n"
      "vxr+oEnx6YBfOYUjvToWNRAt8iG5XRoUDK1fsCPd4Qfw+O+tBHe/mb1VuoELWdvk\n"
      "ITKYpnitRhj/B+nkbf3z1f6wLuKCVM15TTrLRAqQDq16Ho91cV3y5z58Y4w7OafT\n"
      "cPAa4JAniPG535xoClFJUD30LnZKM4Zl57IDnjC5loTzQvD8TZdOsv/Y7sDn9kJU\n"
      "cLjeGkWVhi2msuLdSrUX45iyV4KgbFYKoSJLxMXDM4iaXAm5eNTFGyoYpY6H4rJG\n"
      "juftroQZpX34B4svm4Q=\n"
      "-----END CERTIFICATE-----";

  return tmp_cert;
}

string SignerPoolManager::signMessage(string message) {
  const string private_key_pem =
      "-----BEGIN RSA PRIVATE KEY-----\n"
      "MIIEowIBAAKCAQEAlUYkWp0dlk38+NpPv14UWKnWXQCToGPMNgW75/mUza5vwFcv\n"
      "84zJ+6vaCdstAqJ4RZ92nifD8U2JdvqeuYzGb41tLsTQHYZ5yQKsf2LuXTAX4RJ6\n"
      "YoRut9zbHteKYBKrk5WWZvHn2syFYSbTFF0jHQfNZsJ2lm75hYcaHXVx6weQk0LX\n"
      "KLX9FTfMxk9TEixwCMQLKZfiTdLyaQVQtGE5jAzJ1RlARRIGBTBdpEYBIklaGao4\n"
      "l/UGtGAkp8zuFcSZV4MI7CV9sKvpleOo3EcqaqAuYyPj1e6GKZpZhyR4ZgfV+RDp\n"
      "pHLpfMyJF9AWUa23YJBtTlHSZ8bK2ob93IgKWwIDAQABAoIBAFm1r79tUQy1jeSY\n"
      "fvjJN30ZhKSb5hysHVwSEh2VluINhUeYnk7GO9UuMHlf6Sr+LT3aWdUunMl2Cbkh\n"
      "YSat/cuouQc0bMZxvNsa6kQcVphWgONY0YhHgqqo5l8gth6K2eZ2Lc9sbiTGaRo7\n"
      "1PsWBjY6LC0njVFXHEotKXquzGmsUZs3V9AQcsKJaDMUIthnYQ9Y68EuXvcLGN2J\n"
      "JoJD79TUL5ryHPegOkxyZ6d+qg6EtTUDnLc/eNrO9GIIE9/lYTA+nAkRwwhCvb3Y\n"
      "RjWNgWAUWgbGh7JY+sVJliU6gZaQLV8tJVdAC6peJ/YX48jZEuFhW+hsh69AVskQ\n"
      "AXYRLvECgYEA6Nkz4zfYa3ReKcMCn/gThQLg/OVrrm8LqiEO4M0x/QeEVPWPoUFj\n"
      "UKIdMVk9bZrWz8mtTkZ2DndvlW4sLWA8i5BO57/iMhuX3zFKhbKkolCQfmlPub0j\n"
      "q3RJXk24LJuf0XHzFozWyTkffWqzPj3XRkXFtaqegM6sY8Vwx60Tpo8CgYEApB2u\n"
      "RMNM55MsbRMa2MwPQ0xU4rREVC2K0nL6YC1rC/JJvnF454Bkbf04CgA+vDxoujez\n"
      "ERB76gPE2OZtiQGYvPm1So1y4sr50gyUA+MJRiUS282TH+mZg4gWrpzaMFrLNGIh\n"
      "IteDWx+kc2XgSwBse/t5cuY3Xg2UMfUGVfgB5XUCgYAJCeVpvJE6GppNJLTFYOvh\n"
      "DeuN5Zn4e3cIc6AQOebm25PXeHDK4a7pQmG/uKcZyhjsl/eSQkny5c//DPfKCyJO\n"
      "iJuHg3tUVp3enBs4dWbOpjH1tkDSBPWNgkYW0w2DLcdWagX1qiHsTbtbMvkiQWRJ\n"
      "5gt2sdjVRzCJ7vAh0CYjIQKBgFpn10Yd59Fxq3lymslur8G83M39y94rMFv55p7w\n"
      "phVwpNr6G0Y73hcQ9LvdorGC/ZYHdOcjk384EEoDtbdRX9hKHmoxPWZdtfzV3Pu/\n"
      "J66yYY8f0bo0rtJcN1J9KVyVx3bVz7cnzT53UdoX9tSaKSirQR3gYAO5qzdR6OmW\n"
      "s0S5AoGBAKxpSu9ii7pFA8HxYtGKhJdUpwFxYk9qupsSPyMicsHk4xmsB4bKXUo4\n"
      "npqtfgFr234vIIyQOyAquH9m0i+Lcm6yUaphXR8xBln0bE5h+f7Xv/5lyfxg8Qrj\n"
      "ZtQ6xLoB8F6AThXv63wp9n+kgeden0l6dBy9zPIgtAGiOJa0AGCm\n"
      "-----END RSA PRIVATE KEY-----";

  string label;
  auto decoded_private_key = Botan::PEM_Code::decode(private_key_pem, label);
  Botan::AlgorithmIdentifier algo_identifier(
      "RSA/EMSA3(SHA-256)",
      Botan::AlgorithmIdentifier::Encoding_Option::USE_NULL_PARAM);
  Botan::RSA_PrivateKey private_key(algo_identifier, decoded_private_key);

  Botan::AutoSeeded_RNG rng;
  Botan::PK_Signer pk_signer(private_key, rng, "EMSA3(SHA-256)");

  vector<uint8_t> message_vector(message.begin(), message.end());
  auto signature = pk_signer.sign_message(message_vector, rng);

  return Botan::base64_encode(signature);
}
} // namespace gruut