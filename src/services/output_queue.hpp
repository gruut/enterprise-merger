#pragma once

#include "../chain/types.hpp"
#include "../utils/template_singleton.hpp"
#include <deque>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

namespace gruut {

struct OutputMsgEntry {
  MessageType type;
  nlohmann::json body;
  std::vector<id_type> receivers;
  OutputMsgEntry()
      : type(MessageType::MSG_NULL), body(nullptr), receivers({}) {}
  OutputMsgEntry(MessageType msg_type_, nlohmann::json &msg_body_,
                 std::vector<id_type> &msg_receivers_)
      : type(msg_type_), body(msg_body_), receivers(msg_receivers_) {}
  OutputMsgEntry(MessageType msg_type_, nlohmann::json &msg_body_)
      : type(msg_type_), body(msg_body_), receivers({}) {}
};

class OutputQueueAlt : public TemplateSingleton<OutputQueueAlt> {
private:
  std::deque<OutputMsgEntry> m_output_msg_pool;
  std::mutex m_queue_mutex;

public:
  void push(std::tuple<MessageType, nlohmann::json, std::vector<id_type>>
                &msg_entry_tuple) {
    OutputMsgEntry tmp_msg_entry(std::get<0>(msg_entry_tuple),
                                 std::get<1>(msg_entry_tuple),
                                 std::get<2>(msg_entry_tuple));

    push(tmp_msg_entry);
  }

  void push(MessageType msg_type, nlohmann::json &msg_body) {
    std::vector<id_type> msg_receivers;
    OutputMsgEntry tmp_msg_entry(msg_type, msg_body, msg_receivers);
    push(tmp_msg_entry);
  }

  void push(MessageType msg_type, nlohmann::json &msg_body,
            std::vector<id_type> &msg_receivers) {
    OutputMsgEntry tmp_msg_entry(msg_type, msg_body, msg_receivers);
    push(tmp_msg_entry);
  }

  void push(OutputMsgEntry &msg_entry) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_output_msg_pool.emplace_back(msg_entry);
    m_queue_mutex.unlock();
  }

  OutputMsgEntry fetch() {
    OutputMsgEntry ret_msg;
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (!m_output_msg_pool.empty()) {
      ret_msg = m_output_msg_pool.front();
      m_output_msg_pool.pop_front();
    }
    m_queue_mutex.unlock();
    return ret_msg;
  }

  inline bool empty() { return m_output_msg_pool.empty(); }

  inline void clearOutputQueue() { m_output_msg_pool.clear(); }
};
} // namespace gruut