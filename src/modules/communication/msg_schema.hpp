#ifndef GRUUT_ENTERPRISE_MERGER_MSG_SCHEMA_HPP
#define GRUUT_ENTERPRISE_MERGER_MSG_SCHEMA_HPP

#include "nlohmann/json.hpp"
#include <map>
namespace gruut {
const json SCHEMA_UP = R"({
  "title": "Up",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "ver": {
       "type": "string"
    },
    "cID": {
       "type": "string"
    },
    "ip": {
       "type": "string"
    },
    "port": {
       "type": "string"
    },
    "mCert": {
       "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "ver",
    "cID",
    "ip",
    "port",
    "mCert"
  ]
})"_json;
const json SCHEMA_PING = R"({
  "title": "Ping",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "sCnt": {
      "type": "string"
    },
    "stat": {
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
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "mCert": {
      "type": "string"
    },
    "hgt": {
      "type": "string"
    },
    "prevHash": {
      "type": "string"
    },
    "hash": {
      "type": "string"
    },
    "mSig": {
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
const json SCHEMA_WELCOME = R"({
  "title": "welcome",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "val": {
      "type": "boolean"
    }
  },
  "required": [
    "mID",
    "time",
    "val"
  ]
})"_json;
const json SCHEMA_BLOCK = R"({
  "title": "Block",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "blockraw": {
      "type": "string"
    },
    "tx": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "txid": {
            "type": "string"
          },
          "time": {
            "type": "string"
          },
          "rID": {
            "type": "string"
          },
          "type": {
            "type": "string"
          },
          "content": {
            "type": "array",
            "item": {
              "type": "string"
            }
          },
          "rSig": {
            "type": "string"
          }
        },
        "required": [
          "txid",
          "time",
          "rID",
          "type",
          "content",
          "rSig"
        ]
      }
    }
  },
  "required": [
    "blockraw",
    "tx"
  ]
})"_json;
const json SCHEMA_JOIN = R"({
  "title": "Join",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "ver": {
      "type": "string"
    },
    "cID": {
      "type": "string"
    }
  },
  "required": [
    "sID",
    "time",
    "ver",
    "cID"
  ]
})"_json;
const json SCHEMA_RESPONSE_FIRST = R"({
  "title": "Response 1 to Challenge",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
     "cert": {
      "type": "string"
    },
    "sN": {
      "type": "string"
    },
    "dhx": {
      "type": "string"
    },
    "dhy": {
      "type": "string"
    },
    "sig": {
      "type": "string"
    }
  },
  "required": [
    "sID",
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
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
     "val": {
      "type": "boolean"
    }
  },
  "required": [
    "sID",
    "time",
    "val"
  ]
})"_json;
const json SCHEMA_REQ_SSIG = R"({
  "title": "Partial Block",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
    "mID": {
      "type": "string"
    },
    "cID": {
      "type": "string"
    },
    "hgt": {
      "type": "string"
    },
    "txrt": {
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
const json SCHEMA_SSIG = R"({
  "title": "Signer's Signature",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "sig": {
      "type": "string"
    }
  },
  "required": [
    "sID",
    "time",
    "sig"
  ]
})"_json;
const json SCHEMA_ERROR = R"({
  "title": "Error",
  "type": "object",
  "properties": {
    "sender": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "code": {
      "type": "string"
    },
    "info": {
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "type"
]
})"_json;
const json SCHEMA_TX = R"({
  "title": "Transaction",
  "type": "object",
  "properties": {
    "txid": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "rID": {
      "type": "string"
    },
    "type": {
      "type": "string"
    },
    "content": {
      "type": "array",
      "item": {
        "type": "string"
      }
    },
    "rSig": {
      "type": "string"
    }
  },
  "required": [
    "txid",
    "time",
    "rID",
    "type",
    "content",
    "rSig"
  ]
})"_json;
const json SCHEMA_REQ_CHECK = R"({
  "title": "Request Check",
  "type": "object",
  "properties": {
    "sender": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "dID": {
      "type": "string"
    },
    "txid": {
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "dID",
    "txid"
  ]
})"_json;

const json SCHEMA_REQ_STATUS = R"({
  "title": "Request Status",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time"
  ]
})"_json;

const json SCHEMA_RES_STATUS = R"({
  "title": "Response Status",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "mCert": {
      "type": "string"
    },
    "hgt": {
      "type": "string"
    },
    "hash": {
      "type": "string"
    },
    "mSig": {
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "mCert",
    "hgt",
    "hash",
    "mSig"
  ]
})"_json;

const std::map<MessageType, json> MSG_SCHEMA_MAP = {
    {MessageType::MSG_UP, SCHEMA_UP},
    {MessageType::MSG_PING, SCHEMA_PING},
    {MessageType::MSG_WELCOME, SCHEMA_WELCOME},
    {MessageType::MSG_REQ_BLOCK, SCHEMA_REQ_BLOCK},
    {MessageType::MSG_BLOCK, SCHEMA_BLOCK},
    {MessageType::MSG_JOIN, SCHEMA_JOIN},
    {MessageType::MSG_RESPONSE_1, SCHEMA_RESPONSE_FIRST},
    {MessageType::MSG_SUCCESS, SCHEMA_SUCCESS},
    {MessageType::MSG_REQ_SSIG, SCHEMA_REQ_SSIG},
    {MessageType::MSG_SSIG, SCHEMA_SSIG},
    {MessageType::MSG_ERROR, SCHEMA_ERROR},
    {MessageType::MSG_TX, SCHEMA_TX},
    {MessageType::MSG_REQ_CHECK, SCHEMA_REQ_CHECK},
    {MessageType::MSG_REQ_STATUS, SCHEMA_REQ_STATUS},
    {MessageType::MSG_RES_STATUS, SCHEMA_RES_STATUS}};

class MessageSchema {
public:
  static json getSchema(MessageType msg_type) {
    auto it_map = MSG_SCHEMA_MAP.find(msg_type);
    if (it_map == MSG_SCHEMA_MAP.end()) {
      return json::object();
    } else {
      return it_map->second;
    }
  }
};

}; // namespace gruut

#endif
