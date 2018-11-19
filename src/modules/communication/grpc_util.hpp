#pragma once
#include<string>
#include<iostream>
#include <lz4.h>
#include <cstring>
#include "../../application.hpp"
#include "../../../include/nlohmann/json.hpp"

namespace gruut{
    constexpr uint8_t G = 0x47;
    constexpr uint8_t VERSION = 0x11;
    constexpr uint8_t MAC = 0x02;
    constexpr uint8_t COMPRESSION_TYPE = 0x01;
    constexpr uint8_t NOT_USED = 0x00;
    constexpr uint8_t RESERVED[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF};
    constexpr uint64_t LOCAL_CHAIN_ID = 0xFF;
    constexpr uint64_t SENDER = 0x77;

    constexpr size_t HEADER_LENGTH = 32;

    class Compressor {
    public:
        static int compressData(std::string &src, std::string &dest) {
            size_t src_size = src.size();
            dest.resize(src_size);
            return LZ4_compress_default((const char *) (src.data()), (char *) (dest.data()) ,src_size, src_size);
        }
        static int decompressData(std::string &src, std::string &dest, int origin_size) {
            dest.resize(origin_size);
            return LZ4_decompress_fast((const char *) (src.data()), (char *)(dest.data()), origin_size);
        }
    };
    class HeaderController {
    public:
        static std::string attachHeader(std::string &compressed_json, MessageType message_type);
        static std::string detachHeader(std::string &raw_data);
        static bool validateMessage(std::string &raw_data);
        static int getJsonSize(std::string &raw_data);
        static uint8_t getMessageType(std::string &raw_data);
        static uint8_t getCompressionType(std::string &raw_data);

    };
    class JsonValidator{
    public:
        static bool validateSchema(nlohmann::json json_object, MessageType msg_type);
    };
}