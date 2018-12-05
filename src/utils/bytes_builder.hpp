#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <sstream>
#include <vector>

#include <iostream>

#include "../../include/base64.hpp"

class BytesBuilder {
private:
  std::vector<uint8_t> m_bytes;
  size_t m_filled_len{0};
  bool m_auto_append{true};

public:
  BytesBuilder() {}

  BytesBuilder(size_t len) {
    m_bytes.resize(len);
    m_auto_append = false;
  }

  ~BytesBuilder() {}

  void append(std::vector<uint8_t> &bytes_val, int len = -1) {

    if (len < 0 || len > bytes_val.size())
      len = (int)bytes_val.size();

    if (m_auto_append) {
      m_bytes.insert(m_bytes.end(), bytes_val.begin(), bytes_val.begin() + len);
    } else {
      if (m_filled_len >= m_bytes.size()) {
        return; // fully filled!
      }

      if (m_filled_len + len > m_bytes.size()) {
        return; // not enough space
      }

      std::memcpy(m_bytes.data() + m_filled_len, bytes_val.data(), (size_t)len);

      m_filled_len += len;
    }
  }

  void append(time_t time_val, size_t len = 8) {
    if (len > 8)
      len = 8;

    auto int_val = (uint64_t)time_val;

    append(int_val, len);
  }

  void append(uint64_t int_val, size_t len = 8) {
    if (len > sizeof(uint64_t))
      len = sizeof(uint64_t);

    std::vector<uint8_t> bytes(len, 0x00);

    int start_pos = sizeof(uint64_t) - len;

    for (size_t i = len; i > 0; --i) {
      bytes[len - i] = (uint8_t)((int_val >> 8 * (i - 1)) & 0xFF);
    }

    append(bytes);
  }

  void append(const std::string &str_val, int len = -1) {
    if (len < 0 || len > str_val.size())
      len = (int)str_val.size();

    std::vector<uint8_t> bytes(str_val.begin(), str_val.begin() + len);

    append(bytes);
  }

  void appendHex(const std::string &hex_str_val, int len = -1) {

    if (len < 0)
      len = (int)hex_str_val.size() / 2;

    std::vector<uint8_t> bytes((size_t)len);

    for (int i = 0; i < len; ++i) {
      bytes[i] =
          (uint8_t)strtol(hex_str_val.substr(i * 2, 2).c_str(), nullptr, 16);
    }

    append(bytes);
  }

  void appendDec(const std::string &dec_str_val, int len = -1) {
    auto int_val = (uint64_t)stoi(dec_str_val);
    append(int_val, (size_t)len);
  }

  void appendB64(const std::string &b64_str_val, int len = -1) {
    std::string str_val = macaron::Base64::Decode(b64_str_val);
    append(str_val, len);
  }

  std::vector<uint8_t> getBytes() {
    if (m_auto_append || m_filled_len == m_bytes.size())
      return m_bytes;

    std::vector<uint8_t> ret_bytes(m_bytes.begin(),
                                   m_bytes.begin() + m_filled_len);
    return ret_bytes;
  }

  std::string getString() {
    std::string ret_str;

    if (m_auto_append) {
      ret_str.resize(m_bytes.size());
      ret_str.assign(m_bytes.begin(), m_bytes.end());
    } else {
      ret_str.resize(m_filled_len);
      ret_str.assign(m_bytes.begin(), m_bytes.begin() + m_filled_len);
    }

    return ret_str;
  }

  void clear() {
    m_bytes.clear();
    m_filled_len = 0;
    m_auto_append = true;
  }

  void setLength(size_t len) {
    m_bytes.resize(len);
    m_auto_append = false;
  }

private:
};
