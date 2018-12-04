#include "signer_pool.hpp"

namespace gruut {
constexpr size_t NOT_FOUND = static_cast<const size_t>(-1);

void SignerPool::pushSigner(signer_id_type user_id, std::string &pk_cert,
                            Botan::secure_vector<uint8_t> &hmac_key,
                            SignerStatus status) {
  Signer new_signer;
  new_signer.user_id = user_id;
  new_signer.pk_cert = pk_cert;
  new_signer.hmac_key = hmac_key;
  new_signer.status = status;
  new_signer.last_update = std::time(nullptr);

  auto signer_index = find(new_signer.user_id);
  if (signer_index == NOT_FOUND) {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool.emplace_back(new_signer);
    m_push_mutex.unlock();
  } else {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool[signer_index] = new_signer;
    m_push_mutex.unlock();
  }
}

bool SignerPool::updatePkCert(signer_id_type user_id, std::string &pk_cert) {
  auto signer_index = find(user_id);

  if (signer_index == NOT_FOUND) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool[signer_index].pk_cert = pk_cert;
    m_push_mutex.unlock();

    return true;
  }
}

bool SignerPool::updateHmacKey(signer_id_type user_id,
                               hmac_key_type &hmac_key) {
  auto signer_index = find(user_id);

  if (signer_index == NOT_FOUND) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    m_signer_pool[signer_index].hmac_key = hmac_key;
    m_signer_pool[signer_index].last_update =
        static_cast<uint64_t>(std::time(nullptr));
    m_update_mutex.unlock();

    return true;
  }
}

bool SignerPool::updateStatus(uint64_t user_id, SignerStatus status) {
  auto signer_index = find(user_id);

  if (signer_index == NOT_FOUND) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    m_signer_pool[signer_index].status = status;
    m_signer_pool[signer_index].last_update = std::time(nullptr);
    m_update_mutex.unlock();

    return true;
  }
}

bool SignerPool::removeSigner(uint64_t user_id) {
  auto signer_index = find(user_id);

  if (signer_index == NOT_FOUND) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    m_signer_pool.erase((m_signer_pool.begin() + signer_index));
    m_update_mutex.unlock();

    return true;
  }
}

hmac_key_type SignerPool::getHmacKey(uint64_t user_id) {
  auto signer_index = find(user_id);
  if (signer_index == NOT_FOUND)
    return hmac_key_type();

  return m_signer_pool[signer_index].hmac_key;
}

std::string SignerPool::getPkCert(signer_id_type user_id) {
  auto signer_index = find(user_id);
  if (signer_index == NOT_FOUND)
    return "";

  return m_signer_pool[signer_index].pk_cert;
}

size_t SignerPool::getNumSignerBy(SignerStatus status) {
  size_t signer_num = 0;
  for (Signer &entry : m_signer_pool) {
    if (entry.status == status) {
      ++signer_num;
    }
  }

  return signer_num;
}

void SignerPool::clearPool() {
  std::lock_guard<std::mutex> guard(m_push_mutex);
  m_signer_pool.clear();
  m_push_mutex.unlock();
}

const size_t SignerPool::size() { return m_signer_pool.size(); }

size_t SignerPool::find(signer_id_type user_id) {
  for (size_t index = 0; index < m_signer_pool.size(); ++index) {
    if (m_signer_pool[index].user_id == user_id)
      return index;
  }

  return NOT_FOUND;
}

Signer SignerPool::getSigner(int index) { return m_signer_pool[index]; }
} // namespace gruut