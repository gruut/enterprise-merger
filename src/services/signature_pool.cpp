#include "../application.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"

#include <string>

using namespace nlohmann;
using namespace std;

namespace gruut {
using BytesArray = array<uint8_t, 8>;

void SignaturePool::handleMessage(signer_id_type receiver_id,
                                  json message_body_json) {
  if (verifySignature(receiver_id, message_body_json)) {
    Signature s;

    s.signer_id = receiver_id;

    string signer_sig = message_body_json["sig"].get<string>();
    auto decoded_signer_sig =
        TypeConverter::toBytes(Botan::base64_decode(signer_sig));
    s.signer_signature = decoded_signer_sig;

    push(s);
  }
}

void SignaturePool::push(Signature &signature) {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_signature_pool.emplace_back(signature);
}

size_t SignaturePool::size() { return 0; }

bool SignaturePool::empty() { return size() == 0; }

bool SignaturePool::verifySignature(signer_id_type receiver_id,
                                    json message_body_json) {
  auto pk_cert = Application::app().getSignerPool().getPkCert(receiver_id);
  if (pk_cert != "") {
    bytes signature_message_bytes;

    auto not_decoded_id = message_body_json["sID"].get<string>();
    auto signer_id_str =
        TypeConverter::toString(Botan::base64_decode(not_decoded_id));

    BytesArray signer_id_array = TypeConverter::to8BytesArray(signer_id_str);
    signature_message_bytes.insert(signature_message_bytes.cend(),
                                   signer_id_array.cbegin(),
                                   signer_id_array.cend());

    auto timestamp = message_body_json["time"].get<string>();
    auto timestamp_bytes = TypeConverter::to8BytesArray(timestamp);
    signature_message_bytes.insert(signature_message_bytes.cend(),
                                   timestamp_bytes.cbegin(),
                                   timestamp_bytes.cend());

    PartialBlock &partial_block = Application::app().getTemporaryPartialBlock();
    auto merger_id = partial_block.sender_id;
    signature_message_bytes.insert(signature_message_bytes.cend(),
                                   merger_id.cbegin(), merger_id.cend());

    auto height = partial_block.height;
    BytesArray height_array = TypeConverter::to8BytesArray(height);
    signature_message_bytes.insert(signature_message_bytes.cend(),
                                   height_array.cbegin(), height_array.cend());

    auto tx_root = partial_block.transaction_root;
    signature_message_bytes.insert(signature_message_bytes.cend(),
                                   tx_root.cbegin(), tx_root.cend());

    auto signer_signature_str = message_body_json["sig"].get<string>();
    auto signer_signature_bytes =
        TypeConverter::toBytes(Botan::base64_decode(signer_signature_str));

    bool verify_result = RSA::doVerify(pk_cert, signature_message_bytes,
                                       signer_signature_bytes, true);

    return verify_result;
  }

  return false;
}
} // namespace gruut
