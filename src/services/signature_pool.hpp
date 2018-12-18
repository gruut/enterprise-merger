#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_POOL_HPP

#include <list>
#include <mutex>
#include <nlohmann/json.hpp>

#include "../chain/signature.hpp"

using namespace nlohmann;

namespace gruut {
using Signatures = std::vector<Signature>;

class SignaturePool {
public:
  void handleMessage(signer_id_type &receiver_id, nlohmann::json &);

  void push(Signature &signature);

  bool empty();

  size_t size();

  Signatures fetchN(size_t n);

  inline void clear() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_signature_pool.clear();
  }

private:
  bool verifySignature(signer_id_type &, json &);

  std::list<Signature> m_signature_pool;

  std::mutex m_mutex;
};
}; // namespace gruut
#endif
