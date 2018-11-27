#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP

#include "../chain/signer.hpp"
#include <cstddef>
#include <vector>

using namespace std;

namespace gruut {
using Signers = vector<Signer>;

class SignerPool {
public:
  size_t size();
  bool isFull();
  Signer get(unsigned int index);
  Signers fetchAll();
  void insert(Signer &&signer);

private:
  Signers m_signers;
};
} // namespace gruut

#endif
