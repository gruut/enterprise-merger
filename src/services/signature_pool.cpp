#include "../application.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/type_converter.hpp"

#include "easy_logging.hpp"

#include <string>

using namespace nlohmann;
using namespace std;

namespace gruut {

SignaturePool::SignaturePool() {
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  m_my_chain_id = setting->getLocalChainId();
  el::Loggers::getLogger("SPOL");
}

void SignaturePool::handleMessage(json &msg_body_json) {
  signer_id_type signer_id = Safe::getBytesFromB64(msg_body_json, "sID");

  if (verifySignature(signer_id, msg_body_json)) {
    Signature ssig;

    ssig.signer_id = signer_id;
    ssig.signer_signature = Safe::getBytesFromB64(msg_body_json, "sig");

    push(ssig);

  } else {
    CLOG(ERROR, "SPOL") << "Invalid Support Signature";
  }
}

void SignaturePool::setupSigPool(block_height_type chain_height,
                                 timestamp_type block_time, sha256 &tx_root) {
  m_height = chain_height;
  m_block_time = block_time;
  m_tx_root = tx_root;
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

bool SignaturePool::verifySignature(signer_id_type &recv_id,
                                    json &msg_body_json) {
  auto pk_cert = Application::app().getSignerPool().getPkCert(recv_id);
  if (pk_cert.empty()) {
    auto storage = Storage::getInstance();
    pk_cert = storage->findCertificate(recv_id);
  }

  if (pk_cert.empty()) {
    CLOG(ERROR, "SPOL") << "Empty user certificate";
    return false;
  }

  signer_id_type signer_id = Safe::getBytesFromB64(msg_body_json, "sID");

  BytesBuilder msg_builder;
  msg_builder.append(signer_id);
  msg_builder.append(m_block_time);
  msg_builder.append(m_my_id);
  msg_builder.append(m_my_chain_id);
  msg_builder.append(m_height);
  msg_builder.append(m_tx_root);

  auto sig_bytes = Safe::getBytesFromB64(msg_body_json, "sig");

  return RSA::doVerify(pk_cert, msg_builder.getBytes(), sig_bytes, true);
}
} // namespace gruut
