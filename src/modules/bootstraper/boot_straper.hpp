#pragma once

#include "../../services/output_queue.hpp"
#include "block_synchronizer.hpp"
#include "nlohmann/json.hpp"

#include <functional>
#include <iostream>
#include <string>

#define MY_CID "12312312313"
#define MY_MID "12312312313"
#define MY_VER "0.5.20181128"

namespace gruut {

class BootStraper {
private:
  OutputQueueAlt *m_outputQueue;
  BlockSynchronizer m_block_synchronizer;

public:
  BootStraper() { m_outputQueue = OutputQueueAlt::getInstance(); }
  ~BootStraper() {}

  bool sendMsgUp() {
    nlohmann::json msg_up = {{"mID", MY_MID},
                             {"time", std::to_string(std::time(nullptr))},
                             {"ver", MY_VER},
                             {"cID", MY_CID}};

    m_outputQueue->push(MessageType::MSG_UP, msg_up);
  }

  bool startSync() {
    m_block_synchronizer.startBlockSync(
        std::bind(&BootStraper::endSync, this, std::placeholders::_1));
  }

  void endSync(int x) {
    sendMsgUp();
    // TODO : BPscheduler, MessageFetcher 구동
  }
};
} // namespace gruut