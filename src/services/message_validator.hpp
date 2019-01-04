#pragma once

#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/safe.hpp"
#include "../utils/time.hpp"
#include "easy_logging.hpp"
#include "input_queue.hpp"
#include "nlohmann/json.hpp"
#include <climits>
#include <cmath>
#include <cstdlib>
#include <regex>
#include <string>
#include <tuple>

namespace gruut {

enum class EntryType {
  BASE64,
  TIMESTAMP,
  HEX,
  STRING,
  DECIMAL,
  UINT,
  ARRAYOFOBJECT,
  ARRAYOFSTRING
};

enum class EntryLength : int {
  ID = 12,        // AAAAAAAAAAE=
  SIG_NONCE = 44, // luLSQgVDnMyZjkAh8h2HNokaY1Oe9Md6a4VpjcdGgzs=
  DIFFIE_HELLMAN =
      64, // 92943e52e02476bd1a4d74c2498db3b01c204f29a32698495b4ed0a274e12294
  TX_ID = 44, // Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg=
  NONE = 0
};

const std::vector<std::tuple<MessageType, std::string, EntryType, EntryLength>>
    VALID_FILTER = {
        {MessageType::MSG_JOIN, "sID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_JOIN, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},
        {MessageType::MSG_JOIN, "cID", EntryType::BASE64, EntryLength::ID},

        {MessageType::MSG_RESPONSE_1, "sID", EntryType::BASE64,
         EntryLength::ID},
        {MessageType::MSG_RESPONSE_1, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},
        {MessageType::MSG_RESPONSE_1, "sN", EntryType::BASE64,
         EntryLength::SIG_NONCE},
        {MessageType::MSG_RESPONSE_1, "dhx", EntryType::HEX,
         EntryLength::DIFFIE_HELLMAN},
        {MessageType::MSG_RESPONSE_1, "dhy", EntryType::HEX,
         EntryLength::DIFFIE_HELLMAN},

        {MessageType::MSG_SUCCESS, "sID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_SUCCESS, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},

        {MessageType::MSG_TX, "txid", EntryType::BASE64, EntryLength::TX_ID},
        {MessageType::MSG_TX, "time", EntryType::TIMESTAMP, EntryLength::NONE},
        {MessageType::MSG_TX, "rID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_TX, "type", EntryType::STRING, EntryLength::NONE},
        {MessageType::MSG_TX, "content", EntryType::ARRAYOFSTRING,
         EntryLength::NONE},

        {MessageType::MSG_SSIG, "sID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_SSIG, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},

        {MessageType::MSG_PING, "mID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_PING, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},
        {MessageType::MSG_PING, "sCnt", EntryType::DECIMAL, EntryLength::NONE},

        {MessageType::MSG_UP, "mID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_UP, "time", EntryType::TIMESTAMP, EntryLength::NONE},
        {MessageType::MSG_UP, "cID", EntryType::BASE64, EntryLength::ID},

        {MessageType::MSG_REQ_CHECK, "sender", EntryType::BASE64,
         EntryLength::ID},
        {MessageType::MSG_REQ_CHECK, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},
        {MessageType::MSG_REQ_CHECK, "dID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_REQ_CHECK, "txid", EntryType::BASE64,
         EntryLength::TX_ID},

        {MessageType::MSG_BLOCK, "mID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_BLOCK, "tx", EntryType::ARRAYOFOBJECT,
         EntryLength::NONE},

        {MessageType::MSG_REQ_BLOCK, "mID", EntryType::BASE64, EntryLength::ID},
        {MessageType::MSG_REQ_BLOCK, "time", EntryType::TIMESTAMP,
         EntryLength::NONE},
        {MessageType::MSG_REQ_BLOCK, "hgt", EntryType::UINT,
         EntryLength::NONE}};

class MessageValidator {

public:
  MessageValidator() { el::Loggers::getLogger("MVAL"); }

  bool validate(InputMsgEntry &msg_entry) {
    for (auto &val : VALID_FILTER) {
      if (std::get<0>(val) != msg_entry.type)
        continue;

      if (!validEntryType(msg_entry.body, std::get<2>(val), std::get<1>(val)) ||
          !validEntryLen(msg_entry.body, std::get<1>(val), std::get<3>(val)))
        return false;
    }

    return true;
  }

private:
  bool arrayOfObjectIsValid(nlohmann::json &tx_json) {
    return (tx_json.is_array() && !tx_json.empty());
  }

  bool arrayOfStringIsValid(nlohmann::json &content_json) {
    return (content_json.is_array() && !content_json.empty());
  }

  bool validEntryLen(nlohmann::json &msg_body, const std::string &key,
                     const EntryLength &len) {
    if (len == EntryLength::NONE)
      return true;

    return Safe::getString(msg_body, key).length() == static_cast<int>(len);
  }

  bool validEntryType(nlohmann::json &msg_body, const EntryType &type,
                      const std::string &key) {
    std::string temp = Safe::getString(msg_body, key);
    switch (type) {
    case EntryType::BASE64: {
      std::regex rgx(
          "([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)");
      return !temp.empty() && std::regex_match(temp, rgx);
    }
    case EntryType::TIMESTAMP:
      return !temp.empty() && std::all_of(temp.begin(), temp.end(), ::isdigit);
    case EntryType::HEX:
      return !temp.empty() && std::all_of(temp.begin(), temp.end(), ::isxdigit);
    case EntryType::STRING:
      return (temp == TXTYPE_CERTIFICATES || temp == TXTYPE_DIGESTS);
    case EntryType::DECIMAL:
      return !temp.empty() &&
             std::all_of(temp.begin(), temp.end(), ::isdigit) &&
             (0 <= stoi(temp) && stoi(temp) <= config::MAX_SIGNER_NUM);
    case EntryType::UINT:
      return !temp.empty() &&
             std::all_of(temp.begin(), temp.end(), ::isdigit) &&
             (stoi(temp) >= 0);
    case EntryType::ARRAYOFSTRING:
      return arrayOfObjectIsValid(msg_body["content"]);
    case EntryType::ARRAYOFOBJECT:
      return arrayOfStringIsValid(msg_body["tx"]);
    default:
      return true;
    }
  }
};
} // namespace gruut