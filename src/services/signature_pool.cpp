#include "../application.hpp"
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
    auto decoded_signer_sig = Botan::base64_decode(signer_sig);
    auto decoded_signer_sig_bytes =
        bytes(decoded_signer_sig.begin(), decoded_signer_sig.end());
    s.signer_signature = decoded_signer_sig_bytes;

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
    auto signer_id = message_body_json["sID"].get<string>();
    auto timestamp = message_body_json["time"].get<string>();

    auto signer_signature_str = message_body_json["sig"].get<string>();
    auto sec_vector = Botan::base64_decode(signer_signature_str);
    auto signer_signature_bytes = bytes(sec_vector.begin(), sec_vector.end());
    // TODO: Signer쪽 코드 미완성
    //    bool verify_result = RSA::doVerify(pk_cert, signer_signature_str,
    //    signer_signature_bytes, true);

    return true;
  }

  return false;
}
} // namespace gruut
