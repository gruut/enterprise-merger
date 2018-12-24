#ifndef GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP

#include <lz4.h>
#include <string>
#include <vector>

using namespace std;
class Compressor {
public:
  static string compressData(const string &src) {
    int src_size = static_cast<int>(src.size());
    int dst_size = LZ4_compressBound(src_size);
    string dest;
    dest.resize(dst_size);
    int dest_length = LZ4_compress_default(src.data(), (char *)dest.data(),
                                           src_size, dst_size);
    string dest2(dest.begin(), dest.begin() + dest_length);
    return dest2;
  }

  static vector<uint8_t> compressData(const vector<uint8_t> &src) {
    int src_size = static_cast<int>(src.size());
    int dest_size = LZ4_compressBound(src_size);
    vector<uint8_t> dest(dest_size);
    int dest_length = LZ4_compress_default(
        (const char *)src.data(), (char *)dest.data(), src_size, dest_size);
    vector<uint8_t> dest2(dest.begin(), dest.begin() + dest_length);
    return dest2;
  }

  static string decompressData(string &src) {
    int compressed_size = static_cast<int>(src.size());
    int dest_capacity = compressed_size * 3;
    string dest;
    dest.resize(dest_capacity);
    int dest_length = LZ4_decompress_safe(src.data(), (char *)dest.data(),
                                          compressed_size, dest_capacity);
    string dest2 = dest.substr(0, dest_length);
    return dest2;
  }

  static vector<uint8_t> decompressData(vector<uint8_t> &src) {
    int compressed_size = static_cast<int>(src.size());
    int dest_capacity = compressed_size * 3;
    vector<uint8_t> dest(dest_capacity);
    int dest_length =
        LZ4_decompress_safe((const char *)src.data(), (char *)dest.data(),
                            compressed_size, dest_capacity);
    vector<uint8_t> dest2(dest.begin(), dest.begin() + dest_length);
    return dest2;
  }
};

#endif
