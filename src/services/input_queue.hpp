#pragma once

#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <nlohmann/json.hpp>
#include "../utils/template_singleton.hpp"

namespace gruut {

    struct InputMsgEntry {
        uint8_t type;
        nlohmann::json body;
        InputMsgEntry(uint8_t msg_type_, nlohmann::json &msg_body_) :
                type(msg_type_), body(msg_body_) {}
    };

    class InputQueue : public TemplateSingleton<InputQueue> {
    private:
        std::deque<InputMsgEntry> m_input_msg_pool;
        std::mutex m_queue_mutex;
    public:

        void push(std::tuple<uint8_t, nlohmann::json> &msg_entry_tuple) {
            InputMsgEntry tmp_msg_entry(std::get<0>(msg_entry_tuple), std::get<1>(msg_entry_tuple));
            push(tmp_msg_entry);
        }

        void push(uint8_t msg_type, nlohmann::json &msg_body) {
            InputMsgEntry tmp_msg_entry(msg_type, msg_body);
            push(tmp_msg_entry);
        }

        void push(InputMsgEntry &msg_entry) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_input_msg_pool.emplace_back(msg_entry);
            m_queue_mutex.unlock();
        }

        InputMsgEntry fetch() {
            InputMsgEntry ret_msg = m_input_msg_pool.front();
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_input_msg_pool.pop_front();
            m_queue_mutex.unlock();
            return ret_msg;
        }

        void clearOuputQueue() {
            m_input_msg_pool.clear();
        }
    };
}