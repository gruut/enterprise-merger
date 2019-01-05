#include "signer_pool.hpp"

using namespace gruut::config;

namespace gruut {
void SignerPool::pushSigner(signer_id_type &user_id, std::string &pk_cert,
                            Botan::secure_vector<uint8_t> &hmac_key,
                            SignerStatus status) {
  Signer new_signer;
  new_signer.user_id = user_id;
  new_signer.pk_cert = pk_cert;
  new_signer.hmac_key = hmac_key;
  new_signer.status = status;
  new_signer.last_update = Time::now_int();

  auto signer_iter = find(new_signer.user_id);
  if (signer_iter == m_signer_pool.end()) {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool.push_back(new_signer);
    m_push_mutex.unlock();
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    *signer_iter = new_signer;
    m_update_mutex.unlock();
  }
}

bool SignerPool::updatePkCert(signer_id_type &user_id, std::string &pk_cert) {
  auto signer_iter = find(user_id);

  if (signer_iter == m_signer_pool.end()) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    (*signer_iter).pk_cert = pk_cert;
    m_update_mutex.unlock();

    return true;
  }
}

bool SignerPool::updateHmacKey(signer_id_type &user_id,
                               hmac_key_type &hmac_key) {
  auto signer_iter = find(user_id);

  if (signer_iter == m_signer_pool.end()) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    (*signer_iter).hmac_key = hmac_key;
    (*signer_iter).last_update = Time::now_int();
    m_update_mutex.unlock();

    return true;
  }
}

bool SignerPool::updateStatus(signer_id_type &user_id, SignerStatus status) {
  auto signer_iter = find(user_id);

  if (signer_iter == m_signer_pool.end()) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_update_mutex);
    (*signer_iter).status = status;
    (*signer_iter).last_update = Time::now_int();
    m_update_mutex.unlock();

    return true;
  }
}

bool SignerPool::removeSigner(signer_id_type &user_id) {
  auto signer_iter = find(user_id);

  if (signer_iter == m_signer_pool.end()) {
    return false;
  } else {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool.erase(signer_iter);
    m_push_mutex.unlock();

    return true;
  }
}

hmac_key_type SignerPool::getHmacKey(signer_id_type &user_id) {
  auto signer_iter = find(user_id);
  if (signer_iter == m_signer_pool.end())
    return hmac_key_type();

  return (*signer_iter).hmac_key;
}

std::string SignerPool::getPkCert(signer_id_type &user_id) {
  auto signer_iter = find(user_id);
  if (signer_iter == m_signer_pool.end())
    return "";

  return (*signer_iter).pk_cert;
}

size_t SignerPool::getNumSignerBy(SignerStatus status) {
  auto signers_num = std::count_if(
      m_signer_pool.begin(), m_signer_pool.end(),
      [&status](Signer &signer) { return signer.status == status; });

  return signers_num;
}

void SignerPool::clearPool() {
  std::lock_guard<std::mutex> guard(m_push_mutex);
  m_signer_pool.clear();
  m_push_mutex.unlock();
}

const size_t SignerPool::size() { return m_signer_pool.size(); }

std::vector<Signer> SignerPool::getRandomSigners(size_t number) {
  std::vector<Signer> signers;
  signers.reserve(number);

  std::for_each(m_signer_pool.begin(), m_signer_pool.end(),
                [&signers](Signer &signer) {
                  if (signer.status == SignerStatus::GOOD)
                    signers.emplace_back(signer);
                });

  std::shuffle(signers.begin(), signers.end(),
               std::mt19937(std::random_device()()));

  return vector<Signer>(signers.begin(), signers.begin() + number);
}

std::list<Signer>::iterator SignerPool::find(signer_id_type &user_id) {
  auto it = std::find_if(
      m_signer_pool.begin(), m_signer_pool.end(),
      [this, &user_id](Signer &signer) { return signer.user_id == user_id; });

  return it;
}

Signer SignerPool::getSigner(int index) {
  auto it = m_signer_pool.begin();
  std::advance(it, index);
  return *it;
}

} // namespace gruut