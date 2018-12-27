#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_VALIDATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_VALIDATOR_HPP
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/time.hpp"
#include "nlohmann/json.hpp"

namespace gruut {
using namespace nlohmann;
using namespace std;

const map<EntryName, string> ENTRY_NAME_TO_STRING = {
    {EntryName::S_ID, "sID"},        {EntryName::TIME, "time"},
    {EntryName::C_ID, "cID"},        {EntryName::SN, "sN"},
    {EntryName::DHX, "dhx"},         {EntryName::DHY, "dhy"},
    {EntryName::TX_ID, "txid"},      {EntryName::R_ID, "rID"},
    {EntryName::TYPE, "type"},       {EntryName::M_ID, "mID"},
    {EntryName::S_CNT, "sCnt"},      {EntryName::SENDER, "sender"},
    {EntryName::D_ID, "dID"},        {EntryName::HGT, "hgt"},
    {EntryName::CONTENT, "content"}, {EntryName::TX, "tx"}};

const vector<EntryName> MSG_JOIN_INFO = {EntryName::S_ID, EntryName::TIME,
                                         EntryName::C_ID};
const vector<EntryName> MSG_RESPONSE_1_INFO = {EntryName::S_ID, EntryName::TIME,
                                               EntryName::SN, EntryName::DHX,
                                               EntryName::DHY};
const vector<EntryName> MSG_SUCCESS_INFO = {EntryName::S_ID, EntryName::TIME};
const vector<EntryName> MSG_TX_INFO = {EntryName::TX_ID, EntryName::TIME,
                                       EntryName::R_ID, EntryName::TYPE,
                                       EntryName::CONTENT};
const vector<EntryName> MSG_SSIG_INFO = {EntryName::S_ID, EntryName::TIME};
const vector<EntryName> MSG_PING_INFO = {EntryName::M_ID, EntryName::TIME,
                                         EntryName::S_CNT};
const vector<EntryName> MSG_UP_INFO = {EntryName::M_ID, EntryName::TIME,
                                       EntryName::C_ID};
const vector<EntryName> MSG_REQ_CHECK_INFO = {
    EntryName::SENDER, EntryName::TIME, EntryName::D_ID, EntryName::TX_ID};
const vector<EntryName> MSG_BLOCK_INFO = {EntryName::M_ID, EntryName::TX};
const vector<EntryName> MSG_REQ_BLOCK_INFO = {EntryName::M_ID, EntryName::TIME,
                                              EntryName::HGT};

class MessageValidator {
public:
  static string getEntryName(EntryName entry_name) {
    string name;
    auto it_map = ENTRY_NAME_TO_STRING.find(entry_name);
    if (it_map != ENTRY_NAME_TO_STRING.end())
      name = it_map->second;
    return name;
  }

  inline static bool idValidate(const string &id) {
    return id.length() == static_cast<int>(EntryLength::ID);
  }

  inline static bool txIdValidate(const string &txid) {
    return txid.length() == static_cast<int>(EntryLength::TX_ID);
  }

  inline static bool signerNonceValidate(const string &s_n) {
    return s_n.length() == static_cast<int>(EntryLength::SIG_NONCE);
  }

  inline static bool diffieHellmanValidate(const string &dh) {
    return dh.length() == static_cast<int>(EntryLength::DIFFIE_HELLMAN);
  }

  inline static bool typeValidate(const string &type) {
    return (type == TXTYPE_CERTIFICATES || type == TXTYPE_DIGESTS);
  }

  inline static bool signerCountValidate(const string &my_signer_cnt) {
    return ((0 < stoi(my_signer_cnt)) &&
            (stoi(my_signer_cnt) <= config::MAX_SIGNER_NUM));
  }

  inline static bool heightValidate(const string &hgt) {
    return (stoi(hgt) == -1 || stoi(hgt) > 0);
  }

  static bool timeValidate(MessageType message_type, const string &my_time) {
    auto time_difference = abs(stoll(my_time) - stoll(Time::now()));
    switch (message_type) {
    case MessageType::MSG_JOIN:
      return (0 <= time_difference &&
              time_difference < config::JOIN_TIMEOUT_SEC);
    default:
      return (0 <= time_difference && time_difference < config::MAX_WAIT_TIME);
    }
  }

  static bool contentIdValidate(json content_json) {
    if (!content_json.is_array())
      return false;
    for (size_t i = 0; i < content_json.size(); i += 2) {
      if (!idValidate(content_json[i].get<string>()))
        return false;
    }
    return true;
  }

  static bool entryValidate(MessageType message_type,
                            vector<EntryName> message_type_info,
                            json message_body_json) {
    for (auto &item : message_type_info) {
      string entry_name = getEntryName(item);
      if (entry_name.empty())
        return false;

      if (item == EntryName::CONTENT) { // MSG_TX의 content 처리
        if (!contentIdValidate(message_body_json[entry_name]))
          return false;
        else
          continue;
      }

      if (item == EntryName::TX) { // MSG_BLOCK의 tx 처리
        if (!message_body_json[entry_name].is_array())
          return false;
        for (size_t i = 0; i < message_body_json[entry_name].size(); ++i) {
          if (!entryValidate(message_type, MSG_TX_INFO,
                             message_body_json[entry_name][i]))
            return false;
        }
        continue;
      }

      string entry_value = message_body_json[entry_name].empty()
                               ? ""
                               : message_body_json[entry_name].get<string>();
      if (entry_value.empty())
        return false;

      bool is_entry_valid = true;
      switch (item) {
      case EntryName::S_ID:
      case EntryName::C_ID:
      case EntryName::SENDER:
      case EntryName::D_ID:
      case EntryName::R_ID:
      case EntryName::M_ID:
        is_entry_valid = idValidate(entry_value);
        break;
      case EntryName::TX_ID:
        is_entry_valid = txIdValidate(entry_value);
        break;
      case EntryName::TIME:
        is_entry_valid = timeValidate(message_type, entry_value);
        break;
      case EntryName::SN:
        is_entry_valid = signerNonceValidate(entry_value);
        break;
      case EntryName::DHX:
      case EntryName::DHY:
        is_entry_valid = diffieHellmanValidate(entry_value);
        break;
      case EntryName::TYPE:
        is_entry_valid = typeValidate(entry_value);
        break;
      case EntryName::S_CNT:
        is_entry_valid = signerCountValidate(entry_value);
        break;
      case EntryName::HGT:
        is_entry_valid = heightValidate(entry_value);
        break;
      default:
        break;
      }
      if (!is_entry_valid)
        return false;
    }
    return true;
  }

  static bool validate(MessageType message_type, json message_body_json) {
    switch (message_type) {
    case MessageType::MSG_JOIN:
      return entryValidate(MessageType::MSG_JOIN, MSG_JOIN_INFO,
                           message_body_json);
    case MessageType::MSG_RESPONSE_1:
      return entryValidate(MessageType::MSG_RESPONSE_1, MSG_RESPONSE_1_INFO,
                           message_body_json);
    case MessageType::MSG_SUCCESS:
      return entryValidate(MessageType::MSG_SUCCESS, MSG_SUCCESS_INFO,
                           message_body_json);
    case MessageType::MSG_TX:
      return entryValidate(MessageType::MSG_TX, MSG_TX_INFO, message_body_json);
    case MessageType::MSG_SSIG:
      return entryValidate(MessageType::MSG_SSIG, MSG_SSIG_INFO,
                           message_body_json);
    case MessageType::MSG_PING:
      return entryValidate(MessageType::MSG_PING, MSG_PING_INFO,
                           message_body_json);
    case MessageType::MSG_UP:
      return entryValidate(MessageType::MSG_UP, MSG_UP_INFO, message_body_json);
    case MessageType::MSG_REQ_CHECK:
      return entryValidate(MessageType::MSG_REQ_CHECK, MSG_REQ_CHECK_INFO,
                           message_body_json);
    case MessageType::MSG_BLOCK:
      return entryValidate(MessageType::MSG_BLOCK, MSG_BLOCK_INFO,
                           message_body_json);
    case MessageType::MSG_REQ_BLOCK:
      return entryValidate(MessageType::MSG_REQ_BLOCK, MSG_REQ_BLOCK_INFO,
                           message_body_json);
    default:
      return true;
    }
  }
};
} // namespace gruut
#endif
