#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_POOL_HPP

#include "nlohmann/json.hpp"

#include "../chain/block.hpp"
#include "../chain/signature.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/ecdsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/type_converter.hpp"

#include "../ledger/certificate_ledger.hpp"

#include <list>
#include <mutex>
#include <string>

namespace gruut {
using Signatures = std::vector<Signature>;

class SignaturePool {
public:
  SignaturePool();

  void handleMessage(json &);

  void push(Signature &signature);

  void setupSigPool(block_height_type chain_height, timestamp_type block_time,
                    sha256 &tx_root);

  bool empty();

  size_t size();

  Signatures fetchN(size_t n, block_height_type t_height);

  inline void clear() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_signature_pool.clear();
  }

  void disablePool() { m_enabled = false; }

private:
  void enablePool() { m_enabled = true; }

  bool verifySignature(signer_id_type &, json &);

  std::list<Signature> m_signature_pool;

  merger_id_type m_my_id;
  local_chain_id_type m_my_chain_id;
  block_height_type m_height;
  sha256 m_tx_root;
  timestamp_type m_block_time;
  CertificateLedger m_cert_ledger;

  std::mutex m_mutex;

  std::atomic<bool> m_enabled{false};
};
}; // namespace gruut
#endif
