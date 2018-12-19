#include "../application.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"

#include <string>

using namespace nlohmann;
using namespace std;

namespace gruut {
void SignaturePool::handleMessage(json &message_body_json) {
  string recv_id_b64 = message_body_json["sender"].get<string>();
  signer_id_type receiver_id = TypeConverter::decodeBase64(recv_id_b64);

  if (verifySignature(receiver_id, message_body_json)) {
    Signature s;

    s.signer_id = receiver_id;

    string signer_sig = message_body_json["sig"].get<string>();
    auto decoded_signer_sig = TypeConverter::decodeBase64(signer_sig);
    s.signer_signature = decoded_signer_sig;

    push(s);
  }
}

void SignaturePool::push(Signature &signature) {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_signature_pool.emplace_back(signature);
}

Signatures SignaturePool::fetchN(size_t n) {
  Signatures signatures;
  copy_n(m_signature_pool.begin(), n, back_inserter(signatures));

  return signatures;
}

size_t SignaturePool::size() { return m_signature_pool.size(); }

bool SignaturePool::empty() { return size() == 0; }

bool SignaturePool::verifySignature(signer_id_type &receiver_id,
                                    json &message_body_json) {
  auto pk_cert = Application::app().getSignerPool().getPkCert(receiver_id);
  if (pk_cert.empty()) {
    auto storage = Storage::getInstance();
    pk_cert = storage->findCertificate(receiver_id);
  }

  if (pk_cert.empty()) {
    return false;
  }

  BytesBuilder bytes_builder;

  auto signer_id_b64 = message_body_json["sID"].get<string>();
  signer_id_type signer_id = TypeConverter::decodeBase64(signer_id_b64);
  bytes_builder.append(signer_id);

  auto timestamp = static_cast<timestamp_type>(
      stoll(message_body_json["time"].get<string>()));
  bytes_builder.append(timestamp);

  PartialBlock &partial_block = Application::app().getTemporaryPartialBlock();

  bytes_builder.append(partial_block.merger_id);
  bytes_builder.append(partial_block.chain_id);
  bytes_builder.append(partial_block.height);
  bytes_builder.append(partial_block.transaction_root);

  auto signer_signature_str = message_body_json["sig"].get<string>();
  auto signer_signature_bytes =
      TypeConverter::decodeBase64(signer_signature_str);
  auto signature_message_bytes = bytes_builder.getBytes();

  bool verify_result = RSA::doVerify(pk_cert, signature_message_bytes,
                                     signer_signature_bytes, true);

  return verify_result;
}
} // namespace gruut
