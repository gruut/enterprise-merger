#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>

#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "signer_pool.hpp"

namespace gruut {
using RandomSignerIndices = std::set<int>;

class SignerPoolManager {
public:
  SignerPoolManager() { m_signer_pool = make_shared<SignerPool>(); }
  SignerPool getSelectedSignerPool();
  void putSigner(Signer &&s);
  void handleMessage(MessageType &message_type, uint64_t receiver_id,
                     nlohmann::json message_body_json);

private:
  RandomSignerIndices generateRandomNumbers(unsigned int size);
  bool validateSignature(nlohmann::json message_body_json);

  std::string m_merger_nonce;
  std::shared_ptr<SignerPool> m_signer_pool;
  std::shared_ptr<SignerPool> m_selected_signers_pool;
};
} // namespace gruut
#endif
