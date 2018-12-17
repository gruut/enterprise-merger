#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <algorithm>
#include <array>
#include <botan/base64.h>
#include <botan/secmem.h>
#include <numeric>
#include <string>
#include <vector>

#include "../chain/types.hpp"

using namespace std;

class TypeConverter {
public:
  template <class T>
  inline static std::vector<uint8_t> integerToBytes(T input) {
    std::vector<uint8_t> v;
    v.reserve(sizeof(input));

    for (auto i = 0; i < sizeof(input); ++i) {
      v.push_back(input & 0xFF);
      input >>= 8;
    }

    reverse(v.begin(), v.end());
    return v;
  }

  template <size_t S, typename T = uint8_t>
  inline static std::array<uint8_t, S> integerToArray(T input) {
    using Array = std::array<uint8_t, S>;

    auto bytes = integerToBytes(input);
    Array arr = bytesToArray<S>(bytes);

    return arr;
  }

  template <size_t S>
  inline static std::array<uint8_t, S> bytesToArray(std::vector<uint8_t> b) {
    using Array = std::array<uint8_t, S>;

    Array arr;
    std::copy(b.begin(), b.end(), arr.begin());

    return arr;
  }

  template <size_t S>
  inline static std::vector<uint8_t> arrayToVector(std::array<uint8_t, S> arr) {
    vector<uint8_t> vec(arr.begin(), arr.end());

    return vec;
  }

  inline static std::vector<uint8_t> stringToBytes(const std::string &input) {
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

  inline static std::string digitBytesToIntegerStr(vector<uint8_t> &bytes) {
    auto bit_shift_step = bytes.size() - 1;
    auto num = std::accumulate(bytes.begin(), bytes.end(), 0,
                               [&bit_shift_step](size_t sum, int value) {
                                 sum |= (value << (bit_shift_step * 8));
                                 --bit_shift_step;
                                 return sum;
                               });

    return to_string(num);
  }

  template <typename T> inline static std::string toBase64Str(T &t) {
    return Botan::base64_encode(vector<uint8_t>(begin(t), end(t)));
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
