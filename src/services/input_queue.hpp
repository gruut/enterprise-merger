#pragma once

#include "../chain/types.hpp"
#include "../utils/template_singleton.hpp"
#include "concurrentqueue.hpp"

#include <deque>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

namespace gruut {

struct InputMsgEntry {
  MessageType type;
  nlohmann::json body;
  InputMsgEntry() : type(MessageType::MSG_NULL), body(nullptr) {}
  InputMsgEntry(MessageType msg_type_, nlohmann::json &msg_body_)
      : type(msg_type_), body(msg_body_) {}
};

class InputQueueAlt : public TemplateSingleton<InputQueueAlt> {
private:
  moodycamel::ConcurrentQueue<InputMsgEntry> m_input_msg_pool;

public:
  void push(std::tuple<MessageType, nlohmann::json> &msg_entry_tuple) {
    InputMsgEntry tmp_msg_entry(std::get<0>(msg_entry_tuple),
                                std::get<1>(msg_entry_tuple));
    push(tmp_msg_entry);
  }

  void push(MessageType msg_type, nlohmann::json &msg_body) {
    InputMsgEntry tmp_msg_entry(msg_type, msg_body);
    push(tmp_msg_entry);
  }

  void push(InputMsgEntry &msg_entry) { m_input_msg_pool.enqueue(msg_entry); }

  InputMsgEntry fetch() {
    InputMsgEntry ret_msg;
    m_input_msg_pool.try_dequeue(ret_msg);
    return ret_msg;
  }

  inline size_t size() { return m_input_msg_pool.size_approx(); }

  inline bool empty() { return (m_input_msg_pool.size_approx() == 0); }
};
} // namespace gruut