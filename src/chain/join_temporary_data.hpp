#ifndef GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP
#define GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP

#include <string>
#include <vector>

#include "types.hpp"

using namespace std;

namespace gruut {
struct JoinTemporaryData {
  string merger_nonce;
  string signer_cert;
  vector<uint8_t> shared_secret_key;
  timestamp_type start_time;
};
} // namespace gruut
#endif // GRUUT_ENTERPRISE_MERGER_JOINTEMPORARYDATA_HPP
