#ifndef GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP

#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <botan-2/botan/base64.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/secmem.h>

#include "../chain/types.hpp"

using namespace std;

class TypeConverter {
public:
  template <class T>
  inline static std::vector<uint8_t> integerToBytes(T input_) {

    uint64_t input = input_;
    std::vector<uint8_t> v;
    v.reserve(sizeof(uint64_t));

    for (auto i = 0; i < sizeof(uint64_t); ++i) {
      v.push_back((uint8_t)(input & 0xFF));
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
  inline static std::array<uint8_t, S> bytesToArray(std::vector<uint8_t> &&b) {
    return bytesToArray<S>(b);
  }

  template <size_t S>
  inline static std::array<uint8_t, S> bytesToArray(std::vector<uint8_t> &b) {
    using Array = std::array<uint8_t, S>;

    size_t len = (b.size() >= S) ? S : b.size();

    Array arr;
    std::copy(b.begin(), b.begin() + len, arr.begin());

    return arr;
  }

  template <size_t S>
  inline static std::array<uint8_t, S> base64ToArray(std::string &&b64_str) {
    return base64ToArray<S>(b64_str);
  }

  template <size_t S>
  inline static std::array<uint8_t, S> base64ToArray(std::string &b64_str) {
    using Array = std::array<uint8_t, S>;

    std::vector<uint8_t> decoded_bytes = decodeBase64(b64_str);

    size_t len = (decoded_bytes.size() >= S) ? S : decoded_bytes.size();

    Array arr;
    std::copy(decoded_bytes.begin(), decoded_bytes.begin() + len, arr.begin());

    return arr;
  }

  template <size_t S>
  inline static std::vector<uint8_t>
  arrayToVector(std::array<uint8_t, S> &&arr) {
    return arrayToVector(arr);
  }

  template <size_t S>
  inline static std::vector<uint8_t>
  arrayToVector(std::array<uint8_t, S> &arr) {
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

  static std::vector<uint8_t> digitStringToBytes(std::string &&str) {
    return digitStringToBytes(str);
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

  inline static std::string digitBytesToIntegerStr(vector<uint8_t> &&bytes) {
    return digitBytesToIntegerStr(bytes);
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

  template <typename T> inline static std::string encodeBase64(T &&t) {
    try {
      std::string ret_str =
          Botan::base64_encode(vector<uint8_t>(begin(t), end(t)));
      return ret_str;
    } catch (Botan::Exception &e) {
      std::cout << e.what() << std::endl;
    }
    return std::string("");
  }

  template <typename T>
  inline static std::vector<uint8_t> decodeBase64(T &&input) {
    return decodeBase64(input);
  }

  template <typename T>
  inline static std::vector<uint8_t> decodeBase64(T &input) {
    try {
      auto s_vector = Botan::base64_decode(input);
      return std::vector<uint8_t>(s_vector.begin(), s_vector.end());
    } catch (Botan::Exception &e) {
      std::cout << e.what() << std::endl;
    }

    return std::vector<uint8_t>();
  }
  template <class Container>
  static inline std::string toString(const Container &&bytes) {
    return std::string(bytes.cbegin(), bytes.cend());
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_TYPE_CONVERTER_HPP
