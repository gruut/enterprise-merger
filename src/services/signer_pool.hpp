#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP

#include <array>
#include <deque>
#include <mutex>
#include <string>

#include "../chain/signer.hpp"
#include "../chain/types.hpp"

namespace gruut {
class SignerPool {
public:
  void pushSigner(signer_id_type user_id, std::string &pk_cert,
                  Botan::secure_vector<uint8_t> &hmac_key,
                  SignerStatus stat = SignerStatus::UNKNOWN);

  void createTransactions();

  bool updatePkCert(signer_id_type user_id, std::string &pk_cert);

  bool updateHmacKey(signer_id_type user_id, hmac_key_type &hmac_key);

  bool updateStatus(signer_id_type user_id,
                    SignerStatus status = SignerStatus::GOOD);

  bool removeSigner(signer_id_type user_id);

  hmac_key_type getHmacKey(signer_id_type user_id);

  std::string getPkCert(signer_id_type user_id);

  size_t getNumSignerBy(SignerStatus status = SignerStatus::GOOD);

  Signer getSigner(int index);

  void clearPool();

  const size_t size();

  bool isFull();
  // TODO: May be we should do more operation, but we cannot recognize what
  // operations are required.

private:
  size_t find(signer_id_type user_id);

  std::deque<Signer> m_signer_pool;
  std::mutex m_push_mutex;
  std::mutex m_update_mutex;
};

} // namespace gruut

#endif
