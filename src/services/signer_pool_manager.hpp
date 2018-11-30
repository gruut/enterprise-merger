#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>

#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "signer_pool.hpp"

using namespace std;

namespace gruut {
using RandomSignerIndices = std::set<int>;

class SignerPoolManager {
public:
  SignerPoolManager() { m_signer_pool = make_shared<SignerPool>(); }
  // TODO: SignerPool 구조가 변경됨에 따라 나중에 수정
  //  SignerPool getSelectedSignerPool();
  void handleMessage(MessageType &message_type, uint64_t receiver_id,
                     nlohmann::json message_body_json);

private:
  RandomSignerIndices generateRandomNumbers(unsigned int size);
  bool verifySignature(nlohmann::json message_body_json);
  string getCertificate();
  string signMessage(vector<uint8_t>, vector<uint8_t>, vector<uint8_t>,
                     vector<uint8_t>, vector<uint8_t>);

  bool isJoinable();
  std::string m_merger_nonce;
  std::string m_signer_cert;
  vector<uint8_t> m_shared_secret_key;

  std::shared_ptr<SignerPool> m_signer_pool;
  // TODO: SignerPool 구조가 변경됨에 따라 나중에 수정
  //  std::shared_ptr<SignerPool> m_selected_signers_pool;
};
} // namespace gruut
#endif
