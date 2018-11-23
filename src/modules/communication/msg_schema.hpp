#pragma once
#include "../../../include/nlohmann/json.hpp"
#include "../../application.hpp"
#include <map>
namespace gruut {
using nlohmann::json;
const json SCHEMA_UP = R"({
  "title": "Up",
  "description": "Merger가 Merger Network에 조인할 때",
  "type": "object",
  "properties": {
    "mID": {
      "description": "송신자 Merger",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "ver": {
      "description": "version",
      "type": "string"
    },
    "cID": {
      "description": "local chain ID",
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "ver",
    "cID"
  ]
})"_json;
const json SCHEMA_PING = R"({
  "title": "Ping",
  "description": "Merger가 살아있음을 알림",
  "type": "object",
  "properties": {
    "mID": {
      "description": "송신자 Merger",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "sCnt": {
      "description": "Merger가 연결하고 있는 Signer의 수",
"type": "string"
},
"stat": {
"description": "Merger's status",
"type": "string"
}
},
"required": [
"mID",
"time",
"sCnt",
"stat"
]
})"_json;
const json SCHEMA_REQ_BLOCK = R"({
  "title": "block request",
  "description": "블록 동기화를 위한 블록 요청",
  "type": "object",
  "properties": {
    "mID": {
      "description": "송신자 Merger",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "mCert": {
      "description": "인증서",
      "type": "string"
    },
    "hgt": {
      "description": "block height",
      "type": "string"
    },
    "mSig": {
      "description": "merger의 서명",
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "mCert",
    "hgt",
    "mSig"
  ]
})"_json;
const json SCHEMA_ECHO = R"(
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
const json SCHEMA_JOIN = R"({
  "title": "Join",
  "description": "Signer의 네트워크 참여 요청",
  "type": "object",
  "properties": {
    "sender": {
      "description": "송신자",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "ver": {
      "description": "version",
      "type": "string"
    },
    "cID": {
      "description": "local chain ID",
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "ver",
    "cID"
  ]
})"_json;
const json SCHEMA_RESPONSE_FIRST = R"({
  "title": "Response 1 to Challenge",
  "description": "Signer가 Merger에게 보내는 신원 검증 요청에 대한 응답",
  "type": "object",
  "properties": {
    "sender": {
      "description": "송신자",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
     "cert": {
      "description": "송신자의 인증서",
      "type": "string"
    },
    "sN": {
      "description": "signer's random nonce",
      "type": "string"
    },
    "dhx": {
      "description": "Diffie-Hellman public key x",
      "type": "string"
    },
    "dhy": {
      "description": "Diffie-Hellman public key y",
      "type": "string"
    },
    "sig": {
      "description": "Signer의 nonce, Merger의 nonce, dh1, time으로 만든 서명",
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "cert",
    "sN",
    "dhx",
    "dhy",
    "sig"
  ]
})"_json;
const json SCHEMA_SUCCESS = R"({
  "title": "Success in Key Exchange",
  "description": "Diffie–Hellman 키 교환의 성공을 알림",
  "type": "object",
  "properties": {
    "sender": {
      "description": "송신자",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
     "val": {
      "description": "성공 여부",
      "type": "boolean"
    }
  },
  "required": [
    "sender",
    "time",
    "val"
  ]
})"_json;
const json SCHEMA_LEAVE = R"({
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
})"_json;
const json SCHEMA_SSIG = R"({
  "title": "Partial Block",
  "description": "Merger가 Signer에게 서명을 요청하는 임시블록",
  "type": "object",
  "properties": {
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "mID": {
      "description": "메시지 송신자",
      "type": "string"
    },
    "cID": {
      "description": "chain ID",
      "type": "string"
    },
    "hgt": {
      "description": "block height",
      "type": "string"
    },
    "txrt": {
      "description": "transaction root",
      "type": "string"
    }
  },
  "required": [
    "time",
    "mID",
    "cID",
    "hgt",
    "txrt"
  ]
})"_json;
const json SCHEMA_ERROR = R"({
  "title": "Error",
  "description": "요청 거부 및 오류",
  "type": "object",
  "properties": {
    "sender": {
      "description": "송신자",
      "type": "string"
    },
    "time": {
      "description": "송신 시간",
      "type": "string"
    },
    "type": {
      "description": "Error Type",
"type": "string"
}
},
"required": [
"sender",
"time",
"type"
]
})"_json;

class MessageSchema{
public:
  static json getSchema(MessageType msg_type) {
	return schema_list[msg_type];
  }
private:
  static std::map<MessageType, json> schema_list;
};
std::map<MessageType, json> MessageSchema::schema_list = {
	{MessageType::MSG_UP, SCHEMA_UP},
	{MessageType::MSG_PING, SCHEMA_PING},
	{MessageType::MSG_REQ_BLOCK, SCHEMA_REQ_BLOCK},
	{MessageType::MSG_ECHO, SCHEMA_ECHO},
	{MessageType::MSG_BLOCK, SCHEMA_BLOCK},
	{MessageType::MSG_JOIN, SCHEMA_JOIN},
	{MessageType::MSG_RESPONSE_FIRST, SCHEMA_RESPONSE_FIRST},
	{MessageType::MSG_SUCCESS, SCHEMA_SUCCESS},
	{MessageType::MSG_LEAVE, SCHEMA_LEAVE},
	{MessageType::MSG_SSIG, SCHEMA_SSIG},
	{MessageType::MSG_ERROR, SCHEMA_ERROR}
};
};
