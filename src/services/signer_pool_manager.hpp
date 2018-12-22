#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

#include "../chain/join_temporary_data.hpp"
#include "../chain/signer.hpp"
#include "../chain/types.hpp"

#include "message_proxy.hpp"
#include "signer_pool.hpp"

using namespace std;

namespace gruut {
class SignerPoolManager {
public:
  SignerPoolManager();
  void handleMessage(MessageType &message_type,
                     nlohmann::json &message_body_json);

private:
  bool verifySignature(signer_id_type &signer_id,
                       nlohmann::json &message_body_json);
  string signMessage(string, string, string, string, uint64_t);
  void sendErrorMessage(vector<signer_id_type> &receiver_list,
                        ErrorMsgType error_type, const std::string &info = "");
  bool isJoinable();

  bool isTimeout(std::string &signer_id_b64);

  // A temporary table for connection establishment.
  unordered_map<string, unique_ptr<JoinTemporaryData>> m_join_temp_table;

  string m_my_cert;
  merger_id_type m_my_id;
  MessageProxy m_proxy;
};
} // namespace gruut
#endif
