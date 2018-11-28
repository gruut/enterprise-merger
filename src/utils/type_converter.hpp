#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <string>
#include <vector>

class TypeConverter {
public:
  static std::vector<uint8_t> toBytes(std::string str) {
    return std::vector<uint8_t>(str.begin(), str.end());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
