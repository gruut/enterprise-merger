#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <botan/base64.h>
#include <botan/secmem.h>
#include <string>
#include <vector>

#include "../chain/types.hpp"

class TypeConverter {
public:
  template <class T> inline static std::vector<uint8_t> toBytes(T input) {
    std::vector<uint8_t> v;
    v.reserve(sizeof(input));

    for (auto i = 0; i < sizeof(input); ++i) {
      v.push_back(input & 0xFF);
      input >>= 8;
    }
    return v;
  }

  inline static std::vector<uint8_t> stringToBytes(std::string &input) {
    return std::vector<uint8_t>(input.cbegin(), input.cend());
  }

  inline static Botan::secure_vector<uint8_t>
  toSecureVector(std::vector<uint8_t> &vec) {
    return Botan::secure_vector<uint8_t>(vec.begin(), vec.end());
  }

  static std::vector<uint8_t> digitStringToBytes(std::string &str) {
    uint64_t target_integer = static_cast<uint64_t>(stoll(str));
    std::vector<uint8_t> bytes = {0, 0, 0, 0, 0, 0, 0, 0};

    auto unassigned_index = bytes.size() - 1;
    while (target_integer != 0) {
      uint8_t byte = static_cast<uint8_t>(target_integer & 0xFF);
      bytes[unassigned_index] = byte;

      target_integer >>= 8;
      --unassigned_index;
    }

    return bytes;
  }

  template <typename T> inline static std::string toBase64Str(T &t) {
    return Botan::base64_encode(std::vector<uint8_t>(t.begin(), t.end()));
  }

  template <typename T>
  inline static std::vector<uint8_t> decodeBase64(T &input) {
    auto s_vector = Botan::base64_decode(input);

    return std::vector<uint8_t>(s_vector.begin(), s_vector.end());
  }
  template <class Container>
  static inline std::string toString(const Container &bytes) {
    return std::string(bytes.cbegin(), bytes.cend());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
