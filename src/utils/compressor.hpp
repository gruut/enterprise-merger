#ifndef GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_COMPRESSOR_HPP

#include <string>
#include <lz4.h>

using namespace std;
class Compressor {
public:
    static int compressData(string &src, string &dest) {
        int src_size = static_cast<int>(src.size());
        dest.resize(src_size);
        return LZ4_compress_default((const char *) (src.data()), (char *) (dest.data()), src_size, src_size);
    }

    static int decompressData(string &src, string &dest, int origin_size) {
        dest.resize(static_cast<unsigned long>(origin_size));
        return LZ4_decompress_fast((const char *) (src.data()), (char *) (dest.data()), origin_size);
    }
};

#endif
