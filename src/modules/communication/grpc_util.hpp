#pragma once
#include<string>
#include<lz4.h>

namespace gruut{
    constexpr uint8_t G = 0x47;
    constexpr uint8_t VERSION = 0x11;
    constexpr uint8_t MAC = 0x02;
    constexpr uint16_t RESERVED = 0x00;
    constexpr uint64_t LOCAL_CHAIN_ID = 0xFF;
    constexpr uint64_t SENDER = 0x77;
    constexpr size_t HEADER_LENGTH = 26;

    class Compressor {
    public:
        static int compressData(std::string &src, std::string &dest) {
            size_t src_size = src.size();
            dest.resize(src_size);
            return LZ4_compress_default((const char *) src.data(), (char *) (dest.data()),
                    static_cast<int>(src_size), static_cast<int>(src_size));
        }

        static int decompressData(std::string &src, std::string &dest, int origin_size) {
            dest.resize(static_cast<size_t>(origin_size));
            return LZ4_decompress_safe((const char *) src.data(), (char *)(dest.data()), static_cast<int>(src.size()), origin_size);
        }
    };

    class HeaderController {
    public:
        static std::string attachHeader(std::string &compressed_json, uint8_t message_type) {
            std::string header;
            uint32_t total_length = static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

            header+=G;
            header+=VERSION;
            header+=message_type;
            header+=MAC;
            header+=total_length;
            header+=LOCAL_CHAIN_ID;
            header+=SENDER;
            header+=RESERVED;
            return header+compressed_json;
        }
        static std::string detachHeader(std::string &raw_data)
        {
            size_t json_length = raw_data.size() - HEADER_LENGTH;
            std::string json_dump(&raw_data[26], &raw_data[26] + json_length);

            return json_dump;
        }

        static bool validateMessage(std::string &raw_data)
        {
            uint8_t checker = 0xFF;
            uint8_t check_g = static_cast<uint8_t>(raw_data[0]) & checker;
            uint8_t check_version = static_cast<uint8_t >(raw_data[1]) & checker;
            uint8_t check_mac = static_cast<uint8_t>(raw_data[3]) & checker;

            return (G==check_g && VERSION==check_version && MAC==check_mac);
        }
    };
}