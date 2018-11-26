#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include <memory>
#include <set>
#include <vector>

#include "../chain/signer.hpp"
#include "signer_pool.hpp"

namespace gruut {
using RandomSignerIndices = std::set<int>;

class SignerPoolManager {
public:
  SignerPoolManager() { m_signer_pool = make_shared<SignerPool>(); }
  SignerPool getSelectedSignerPool();
  void putSigner(Signer &&s);

private:
  RandomSignerIndices generateRandomNumbers(unsigned int size);
  std::shared_ptr<SignerPool> m_signer_pool;
  std::shared_ptr<SignerPool> m_selected_signers_pool;
};
} // namespace gruut
#endif
