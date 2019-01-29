#ifndef GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#define GRUUT_ENTERPRISE_MERGER_SHA_256_HPP

#include <botan-2/botan/base64.h>
#include <botan-2/botan/hash.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class Sha256 {
  using hash_t = std::vector<uint8_t>;

public:
  static hash_t hash(const string &message) {
    std::vector<uint8_t> v(message.begin(), message.end());
    return hash(v);
  }

  static bool isMatch(const string &target_message,
                      const hash_t &hashed_message) {
    auto hashed_target_message = Sha256::hash(target_message);
    bool result = hashed_target_message == hashed_message;

    return result;
  }

  static string toString(hash_t hashed_list) {
    return Botan::base64_encode(hashed_list);
  }

  static hash_t hash(std::vector<uint8_t> &&data) { return hash(data); }

  static hash_t hash(std::vector<uint8_t> &data) {
    std::unique_ptr<Botan::HashFunction> hash_function(
        Botan::HashFunction::create("SHA-256"));
    hash_function->update(data);
    return hash_function->final_stdvec();
  }

  template <size_t S> static hash_t hash(std::array<uint8_t, S> &data) {
    std::unique_ptr<Botan::HashFunction> hash_function(
        Botan::HashFunction::create("SHA-256"));

    std::vector<uint8_t> data_vec(data.begin(), data.end());

    hash_function->update(data_vec);
    return hash_function->final_stdvec();
  }
};

#endif
