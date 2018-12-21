#include "../application.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"

#include <string>

using namespace nlohmann;
using namespace std;

namespace gruut {
void SignaturePool::handleMessage(json &message_body_json) {
  string recv_id_b64 = message_body_json["sID"].get<string>();
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

  auto signer_id_b64 = message_body_json["sID"].get<string>();
  signer_id_type signer_id = TypeConverter::decodeBase64(signer_id_b64);

  auto timestamp = static_cast<timestamp_type>(
      stoll(message_body_json["time"].get<string>()));

  PartialBlock &partial_block = Application::app().getTemporaryPartialBlock();

  BytesBuilder msg_builder;
  msg_builder.append(signer_id);
  msg_builder.append(timestamp);
  msg_builder.append(partial_block.merger_id);
  msg_builder.append(partial_block.chain_id);
  msg_builder.append(partial_block.height);
  msg_builder.append(partial_block.transaction_root);

  auto sig_b64 = message_body_json["sig"].get<string>();
  auto sig_bytes = TypeConverter::decodeBase64(sig_b64);

  return RSA::doVerify(pk_cert, msg_builder.getBytes(), sig_bytes, true);
}
} // namespace gruut
