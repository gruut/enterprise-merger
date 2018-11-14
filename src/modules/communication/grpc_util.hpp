#pragma once
#include<string>
#include<lz4.h>

namespace gruut{
    const uint8_t G = 0x47;
    const uint8_t VERSION = 0x11;
    const uint8_t MAC = 0x02;
    const uint16_t RESERVED = 0x00;
    const uint64_t LOCAL_CHAIN_ID = 0xFF;

    class Compressor {
    public:
        static int compressData(std::string &src, std::string &dest) {
            int src_size = src.size();
            dest.resize(src_size);
            return LZ4_compress_default((const char *) src.data(), (char *) (dest.data()), src_size, src_size);
        }

        static int decompressData(std::string &src, std::string &dest, int origin_size) {
            dest.resize(origin_size);
            return LZ4_decompress_safe((const char *) src.data(), (char *) (dest.data()), src.size(), origin_size);
        }
    };

    class HeaderController {
    public:
        static std::string attachHeader(std::string &compressed_json, uint8_t message_type, uint64_t sender) {
            std::string header;
            uint32_t total_length = 26 + compressed_json.size();

            header+=G;
            header+=VERSION;
            header+=message_type;
            header+=MAC;
            header+=total_length;
            header+=LOCAL_CHAIN_ID;
            header+=sender;
            header+=RESERVED;
            return header+compressed_json;
        }
    };
}