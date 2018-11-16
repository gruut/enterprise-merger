#pragma once
#include <map>
#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
namespace gruut
{
    using nlohmann::json;
    const json SCHEMA_ECHO= R"(
            {
                "title": "Echo",
                        "description": "노드 간에 네트워크 유지를 위해 보내는 Echo",
                        "type": "object",
                        "properties": {
                    "sender": {
                        "description": "송신자",
                                "type": "string"
                    },
                    "time": {
                        "description": "송신 시간",
                                "type": "string"
                    }
                },
                "required": [
                "sender",
                "time"
                ]
            }
            )"_json;
    const json SCHEMA_BLOCK = R"(
            {
              "title": "Block",
              "description": "Merger가 브로드캐스팅할 블록",
              "type": "object",
              "properties": {
                "blockraw": {
                  "description": "meta, compressed JSON, mSig",
                  "type": "string"
                },
                "secret": {
                  "description": "mtree, txCnt, tx",
                  "type": "string"
                }
              }
            }
            )"_json;

    class MessageSchema{
    public:
        static std::map<MessageType, json> schema_list;
    };
    std::map<MessageType, json> MessageSchema::schema_list = {
            {MessageType ::MSG_ECHO, SCHEMA_ECHO},
            {MessageType::MSG_BLOCK, SCHEMA_BLOCK}
    };

}