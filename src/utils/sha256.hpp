#ifndef GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#define GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#include <botan/base64.h>
#include <botan/hash.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace Botan;

class Sha256 {
  using sha256 = std::vector<uint8_t>;

public:
  static sha256 hash(const string &message) {
    sha256 v(message.begin(), message.end());
    return hash(v);
  }

  static bool isMatch(const string &target_message,
                      const sha256 &hashed_message) {
    auto hashed_target_message = Sha256::hash(target_message);
    bool result = hashed_target_message == hashed_message;

    return result;
  }

  static string toString(sha256 hashed_list) {
    return base64_encode(hashed_list);
  }

  static sha256 hash(std::vector<uint8_t> &data) {
    std::unique_ptr<Botan::HashFunction> hash_function(Botan::HashFunction::create("SHA-256"));
    hash_function->update(data);
    return hash_function->final_stdvec();
  }

};

#endif
