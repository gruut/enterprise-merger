#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <botan/base64.h>
#include <botan/secmem.h>
#include <string>
#include <vector>

#include "../chain/types.hpp"

class TypeConverter {
public:
  template <class T> inline static std::vector<uint8_t> toBytes(T t) {
    return std::vector<uint8_t>(t.cbegin(), t.cend());
  }

  static Botan::secure_vector<uint8_t>
  toSecureVector(std::vector<uint8_t> vec) {
    return Botan::secure_vector<uint8_t>(vec.begin(), vec.end());
  }

  static std::array<uint8_t, 8> to8BytesArray(std::string str) {
    uint64_t target_integer = static_cast<uint64_t>(stoll(str));
    std::array<uint8_t, 8> bytes_arr = {0, 0, 0, 0, 0, 0, 0, 0};

    auto unassigned_index = bytes_arr.size() - 1;
    while (target_integer != 0) {
      uint8_t byte = static_cast<uint8_t>(target_integer & 0xFF);
      bytes_arr[unassigned_index] = byte;

      target_integer >>= 8;
      --unassigned_index;
    }

    return bytes_arr;
  }

  static std::string toBase64Str(vector<uint8_t> &bytes_vector) {
    return Botan::base64_encode(bytes_vector);
  }

  template <class Container>
  static inline std::string toString(const Container &bytes) {
    return std::string(bytes.cbegin(), bytes.cend());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
