#include "signer_pool.hpp"

namespace gruut {
// TODO: 적절한 값 결정해야함. 임시로 100
constexpr int MAX_SIGNER_POOL_SIZE = 100;
size_t SignerPool::size() { return m_signers.size(); }

Signer SignerPool::get(unsigned int index) { return m_signers[index]; }

void SignerPool::insert(Signer &&signer) { m_signers.push_back(signer); }

Signers SignerPool::fetchAll() { return m_signers; }

bool SignerPool::isFull() { return this->size() == MAX_SIGNER_POOL_SIZE; }
} // namespace gruut