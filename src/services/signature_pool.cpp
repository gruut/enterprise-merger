#include "../application.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"

#include <string>

using namespace nlohmann;
using namespace std;

namespace gruut {
void SignaturePool::handleMessage(signer_id_type receiver_id,
                                  json message_body_json) {
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

size_t SignaturePool::size() { return 0; }

bool SignaturePool::empty() { return size() == 0; }

bool SignaturePool::verifySignature(signer_id_type receiver_id,
                                    json message_body_json) {
  auto pk_cert = Application::app().getSignerPool().getPkCert(receiver_id);
  if (pk_cert != "") {
    BytesBuilder bytes_builder;

    auto not_decoded_id = message_body_json["sID"].get<string>();
    auto signer_id_str =
        TypeConverter::toString(TypeConverter::decodeBase64(not_decoded_id));
    auto signer_id_vector = TypeConverter::digitStringToBytes(signer_id_str);
    bytes_builder.append(signer_id_vector);

    auto timestamp = message_body_json["time"].get<string>();
    auto timestamp_bytes = TypeConverter::digitStringToBytes(timestamp);
    bytes_builder.append(timestamp_bytes);

    PartialBlock &partial_block = Application::app().getTemporaryPartialBlock();
    string merger_id_str = to_string(partial_block.merger_id);
    bytes_builder.append(merger_id_str);

    auto height = partial_block.height;
    auto height_vec = TypeConverter::digitStringToBytes(height);
    bytes_builder.append(height_vec);

    auto tx_root = partial_block.transaction_root;
    bytes_builder.append(tx_root);

    auto signer_signature_str = message_body_json["sig"].get<string>();
    auto signer_signature_bytes =
        TypeConverter::decodeBase64(signer_signature_str);
    auto signature_message_bytes = bytes_builder.getBytes();

    bool verify_result = RSA::doVerify(pk_cert, signature_message_bytes,
                                       signer_signature_bytes, true);

    return verify_result;
  }

  return false;
}
} // namespace gruut
