#ifndef GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP

#include <lz4.h>
#include <string>
#include <vector>

using namespace std;
class Compressor {
public:
  static int compressData(string &src, string &dest) {
    int src_size = static_cast<int>(src.size());
    int dst_size = LZ4_compressBound(src_size);
    dest.resize(dst_size);
    return LZ4_compress_default(src.data(), (char *)dest.data(), src_size,
                                dst_size);
  }

  static vector<uint8_t> compressData(vector<uint8_t> &src) {
    int src_size = static_cast<int>(src.size());
    int dest_size = LZ4_compressBound(src_size);
    vector<uint8_t> dest(dest_size);
    int dest_length = LZ4_compress_default(
        (const char *)src.data(), (char *)dest.data(), src_size, dest_size);
    vector<uint8_t> dest2(dest.begin(), dest.begin() + dest_length);
    return dest2;
  }

  static int decompressData(string &src, string &dest, int compressed_size) {
    // TODO: decompressed data의 최대 capacity는 현재 compressed data의 3배로
    // 설정해 놓았습니다. 수정 될수 있습니다.
    int dest_capacity = compressed_size * 3;
    dest.resize(dest_capacity);
    int dest_length = LZ4_decompress_safe(src.data(), (char *)dest.data(),
                                          compressed_size, dest_capacity);
    dest = dest.substr(0, dest_length);
    return dest_length;
  }
  // TODO: 위의 함수는 삭제될 예정입니다. 위의 함수는 현재 사용하고 있는 곳이
  // 있습니다.
  static int decompressData(string &src, string &dest) {
    int compressed_size = static_cast<int>(src.size());
    int dest_capacity = compressed_size * 3;
    dest.resize(dest_capacity);
    int dest_length = LZ4_decompress_safe(src.data(), (char *)dest.data(),
                                          compressed_size, dest_capacity);
    dest = dest.substr(0, dest_length);
    return dest_length;
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
