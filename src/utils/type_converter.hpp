#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <botan/secmem.h>
#include <string>
#include <vector>

class TypeConverter {
public:
  static std::vector<uint8_t> toBytes(std::string str) {
    return std::vector<uint8_t>(str.begin(), str.end());
  }

  static Botan::secure_vector<uint8_t>
  toSecureVector(std::vector<uint8_t> vec) {
    return Botan::secure_vector<uint8_t>(vec.begin(), vec.end());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
