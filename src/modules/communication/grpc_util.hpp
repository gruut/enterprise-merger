#pragma once
#include<string>
#include <lz4.h>
#include "../../application.hpp"

namespace gruut{
    constexpr uint8_t G = 0x47;
    constexpr uint8_t VERSION = 0x11;
    constexpr uint8_t MAC = 0x02;
    constexpr uint16_t RESERVED = 0x00;
    constexpr uint64_t LOCAL_CHAIN_ID = 0xFF;
    constexpr uint64_t SENDER =0x77;
    constexpr size_t HEADER_LENGTH = 26;

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
        static std::string attachHeader(std::string &compressed_json, MessageType message_type) {
            std::string header;
            uint32_t total_length = static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

            header.resize(26);
            header[0]=G;
            header[1]=VERSION;
            header[2]= static_cast<u_int8_t>(message_type);
            header[3]=MAC;
            for(int i=7; i>4; i--) {
                header[i]|=total_length;
                total_length = (total_length>>8);
            }
            header[4] |= total_length;

            uint64_t local_chain_id = LOCAL_CHAIN_ID;
            for(int i=15; i>8; i--){
                header[i]|=local_chain_id;
                local_chain_id =(local_chain_id>>8);
            }
            header[8] |=local_chain_id;

            uint64_t sender_id = SENDER;
            for(int i=23; i>16; i--){
                header[i]|=sender_id;
                sender_id=(sender_id>>8);
            }
            header[15] |= sender_id;

            uint16_t reserved = RESERVED;
            header[25] |= reserved;
            reserved = (reserved>>8);
            header[24] |=reserved;

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
        static int getJsonSize(std::string &raw_data)
        {
            int len = 0;
            for(int i=4; i<7; i++) {
                len|=raw_data[i];
                len=len<<8;
            }
            len|=raw_data[7];
            return len-26;
        }
        static uint8_t getMessageType(std::string &raw_data) {
            uint8_t message_type = 0;
            message_type |= raw_data[2];
            return message_type;
        }
    };
}