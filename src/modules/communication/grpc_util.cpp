#include "grpc_util.hpp"
#include "../../../include/json_schema.hpp"
#include "msg_schema.hpp"
namespace gruut{
    std::string HeaderController::attachHeader(std::string &compressed_json, MessageType message_type) {
        std::string header;
        uint32_t total_length = static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

        header.resize(32);
        header[0] = G;
        header[1] = VERSION;
        header[2] = static_cast<uint8_t>(message_type);
        header[3] = MAC;
        header[4] = COMPRESSION_TYPE;
        header[5] = NOT_USED;
        for(int i=9; i>6; i--) {
            header[i]|=total_length;
            total_length = (total_length>>8);
        }
        header[6] |= total_length;

        uint64_t local_chain_id = LOCAL_CHAIN_ID;
        for(int i=17; i>10; i--){
            header[i]|=local_chain_id;
            local_chain_id =(local_chain_id>>8);
        }
        header[10] |=local_chain_id;

        uint64_t sender_id = SENDER;
        for(int i=25; i>18; i--){
            header[i]|=sender_id;
            sender_id=(sender_id>>8);
        }
        header[18] |= sender_id;

        for(int i=0; i<6; i++) {
            header[26 + i] = RESERVED[i];
        }
        return header+compressed_json;
    }
    std::string HeaderController::detachHeader(std::string &raw_data)
    {
        size_t json_length = raw_data.size() - HEADER_LENGTH;
        std::string json_dump(&raw_data[HEADER_LENGTH], &raw_data[HEADER_LENGTH] + json_length);

        return json_dump;
    }
    bool HeaderController::validateMessage(std::string &raw_data)
    {
        uint8_t paser = 0xFF;
        uint8_t check_g = static_cast<uint8_t>(raw_data[0]) & paser;
        uint8_t check_version = static_cast<uint8_t >(raw_data[1]) & paser;
        uint8_t check_mac = static_cast<uint8_t>(raw_data[3]) & paser;

        return (G==check_g && VERSION==check_version && MAC==check_mac);
    }
    int HeaderController::getJsonSize(std::string &raw_data)
    {
        int len = 0;
        for(int i=6; i<9; i++) {
            len|=raw_data[i];
            len=len<<8;
        }
        len|=raw_data[9];
        return len - static_cast<int>(HEADER_LENGTH);
    }
    uint8_t HeaderController::getMessageType(std::string &raw_data) {
        uint8_t message_type = 0;
        message_type |= raw_data[2];
        return message_type;
    }
    uint8_t HeaderController::getCompressionType(std::string &raw_data){
        uint8_t compress_type = 0;
        compress_type |= raw_data[4];
        return compress_type;
    }
    bool JsonValidator::validateSchema(json json_object, MessageType msg_type){
        using nlohmann::json;
        using nlohmann::json_schema_draft4::json_validator;

        json_validator schema_validator;
        schema_validator.set_root_schema(MessageSchema::getSchema(msg_type));

        try{
            schema_validator.validate(json_object);
            std::cout << "Validation succeeded"<<std::endl;
            return true;
        }
        catch(const std::exception &e) {
            std::cout << "Validation failed : " << e.what() << std::endl;
            return false;
        }
    }
}
