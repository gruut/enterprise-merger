#pragma once

#include "nlohmann/json.hpp"
#include "easy_logging.hpp"

#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/safe.hpp"
#include "../utils/time.hpp"
#include "input_queue.hpp"

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
  ID = 12,              // Base64 64-bit = 4 * ceil(8-byte/3) = 12 chars
  SIG_NONCE = 44,       // Base64 256-bit = 4 * ceil(32-byte/3) = 44 chars
  DIFFIE_HELLMAN = 64,  // Hex 256-bit = 64 chars
  TX_ID = 44,           // Base64 256-bit = 44 chars
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
          !hasValidLength(msg_entry.body, std::get<1>(val), std::get<3>(val)))
        return false;
    }

    return true;
  }

private:

  bool hasValidLength(nlohmann::json &msg_body, const std::string &key,
                      const EntryLength &len) {
    if (len == EntryLength::NONE)
      return true;

    return Safe::getString(msg_body, key).length() == static_cast<int>(len);
  }

  bool validEntryType(nlohmann::json &msg_body, const EntryType &type, const std::string &key) {

    switch (type) {
    case EntryType::BASE64: {
      std::string temp = Safe::getString(msg_body, key);
      std::regex rgx(
          "([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)");
      return !temp.empty() && std::regex_match(temp, rgx);
    }
    case EntryType::TIMESTAMP: {
      std::string temp = Safe::getString(msg_body, key);
      return !temp.empty() && std::all_of(temp.begin(), temp.end(), ::isdigit);
    }
    case EntryType::HEX: {
      std::string temp = Safe::getString(msg_body, key);
      return !temp.empty() && std::all_of(temp.begin(), temp.end(), ::isxdigit);
    }
    case EntryType::STRING: {
      std::string temp = Safe::getString(msg_body, key);
      return (temp == TXTYPE_CERTIFICATES || temp == TXTYPE_DIGESTS);
    }
    case EntryType::DECIMAL: {
      std::string temp = Safe::getString(msg_body, key);
      return !temp.empty() &&
        std::all_of(temp.begin(), temp.end(), ::isdigit) &&
        (0 <= stoi(temp) && stoi(temp) <= config::MAX_SIGNER_NUM);
    }
    case EntryType::UINT: {
      std::string temp = Safe::getString(msg_body, key);
      return !temp.empty() &&
        std::all_of(temp.begin(), temp.end(), ::isdigit) &&
        (stoi(temp) >= 0);
    }
    case EntryType::ARRAYOFSTRING: {
      return (msg_body[key].is_array() && !msg_body[key].empty());
    }
    case EntryType::ARRAYOFOBJECT: {
      return (msg_body[key].is_array() && !msg_body[key].empty());
    }
    default:
      return true;
    }
  }
};
} // namespace gruut