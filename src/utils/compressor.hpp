#ifndef GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP

#include <lz4.h>
#include <string>

using namespace std;
class Compressor {
public:
  static int compressData(string &src, string &dest) {
    int src_size = static_cast<int>(src.size());
    int dst_size = LZ4_compressBound(src_size);
    dest.resize(dst_size);
    return LZ4_compress_default(src.data(), &dest[0], src_size, dst_size);
  }

  static int decompressData(string &src, string &dest, int compressed_size) {
    //TODO: decompressed data의 최대 capacity는 현재 compressed data의 3배로 설정해 놓았습니다. 수정 될수 있습니다.
    int dest_capacity = compressed_size * 3;
    dest.resize(dest_capacity);
    int dest_length = LZ4_decompress_safe(src.data(), &dest[0], compressed_size, dest_capacity);
    dest = dest.substr(0, dest_length);
    return dest_length;
  }
};

#endif
