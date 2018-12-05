#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <botan/base64.h>
#include <botan/secmem.h>
#include <string>
#include <vector>

#include "../chain/types.hpp"

class TypeConverter {
public:
  static std::vector<uint8_t> toBytes(std::string str) {
    return std::vector<uint8_t>(str.begin(), str.end());
  }

  static Botan::secure_vector<uint8_t>
  toSecureVector(std::vector<uint8_t> vec) {
    return Botan::secure_vector<uint8_t>(vec.begin(), vec.end());
  }

  static std::array<uint8_t, 8> toTimestampType(std::string str) {
    uint64_t time_int = static_cast<uint64_t>(stoll(str));
    std::array<uint8_t, 8> timestamp = {0, 0, 0, 0, 0, 0, 0, 0};

    auto unassigned_index = timestamp.size() - 1;
    while (time_int != 0) {
      uint8_t byte = static_cast<uint8_t>(time_int & 0xFF);
      timestamp[unassigned_index] = byte;

      time_int >>= 8;
      --unassigned_index;
    }

    return timestamp;
  }

  static std::string toBase64Str(vector<uint8_t> &bytes_vector) {
    return Botan::base64_encode(bytes_vector);
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
