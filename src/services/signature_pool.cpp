#include "../application.hpp"
#include "easy_logging.hpp"

using namespace std;

namespace gruut {

SignaturePool::SignaturePool() {
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  m_my_chain_id = setting->getLocalChainId();
  el::Loggers::getLogger("SPOL");
}

void SignaturePool::handleMessage(json &msg_body_json) {
  if (!m_enabled) {
    CLOG(ERROR, "SPOL") << "Support Signature dropped (disabled pool)";
    return;
  }

  signer_id_type signer_id = Safe::getBytesFromB64(msg_body_json, "sID");

  if (verifySignature(signer_id, msg_body_json)) {
    Signature ssig;

    ssig.signer_id = signer_id;
    ssig.height = m_height;
    ssig.signer_signature = Safe::getBytesFromB64(msg_body_json, "sig");

    push(ssig);

  } else {
    CLOG(ERROR, "SPOL") << "Invalid Support Signature for This Block";
  }
}

void SignaturePool::setupSigPool(block_height_type chain_height,
                                 timestamp_type block_time, sha256 &tx_root) {
  m_height = chain_height;
  m_block_time = block_time;
  m_tx_root = tx_root;

  enablePool();
}

void SignaturePool::push(Signature &signature) {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_signature_pool.emplace_back(signature);
}

Signatures SignaturePool::fetchN(size_t n, block_height_type t_height) {
  Signatures signatures;
  for (auto &ssig : m_signature_pool) {
    if (ssig.height == t_height) {
      signatures.emplace_back(ssig);
    }

    if (signatures.size() >= n)
      break;
  }

  return signatures;
}

size_t SignaturePool::size() { return m_signature_pool.size(); }

bool SignaturePool::empty() { return size() == 0; }

bool SignaturePool::verifySignature(signer_id_type &recv_id,
                                    json &msg_body_json) {
  auto pk_cert = SignerPool::getInstance()->getPkCert(recv_id);
  if (pk_cert.empty()) {
    auto &certificate_ledger = Application::app().getCertificateLedger();
    pk_cert = certificate_ledger.getCertificate(recv_id);
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

  return ECDSA::doVerify(pk_cert, msg_builder.getBytes(), sig_bytes);
}
} // namespace gruut
