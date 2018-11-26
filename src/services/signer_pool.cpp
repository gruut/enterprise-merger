#include "signer_pool.hpp"

namespace gruut {
size_t SignerPool::size() { return m_signers.size(); }

Signer SignerPool::get(unsigned int index) { return m_signers[index]; }

void SignerPool::insert(Signer &&signer) { m_signers.push_back(signer); }

Signers SignerPool::fetchAll() { return m_signers; }
} // namespace gruut