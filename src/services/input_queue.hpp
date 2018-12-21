#pragma once

#include "../chain/types.hpp"
#include "../utils/template_singleton.hpp"
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
  std::deque<InputMsgEntry> m_input_msg_pool;
  std::mutex m_queue_mutex;

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

  void push(InputMsgEntry &msg_entry) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_input_msg_pool.emplace_back(msg_entry);
    m_queue_mutex.unlock();
  }

  InputMsgEntry fetch() {
    InputMsgEntry ret_msg;
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (!m_input_msg_pool.empty()) {
      ret_msg = m_input_msg_pool.front();
      m_input_msg_pool.pop_front();
    }
    m_queue_mutex.unlock();
    return ret_msg;
  }

  inline size_t size() { return m_input_msg_pool.size(); }

  inline bool empty() { return m_input_msg_pool.empty(); }

  inline void clearInputQueue() { m_input_msg_pool.clear(); }
};
} // namespace gruut