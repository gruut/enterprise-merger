#ifndef GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP
#define GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP

#include <string>
#include <vector>

using namespace std;

namespace gruut {
struct JoinTemporaryData {
  string merger_nonce;
  string signer_cert;
  vector<uint8_t> shared_secret_key;
};
} // namespace gruut
#endif // GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP
