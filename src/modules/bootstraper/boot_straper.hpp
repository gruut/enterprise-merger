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
  ~BootStraper() { m_outputQueue = nullptr; }

  bool sendMsgUp() {
    nlohmann::json msg_up = {{"mID", MY_MID},
                             {"time", std::to_string(std::time(nullptr))},
                             {"ver", MY_VER},
                             {"cID", MY_CID}};
    std::cout << "msg up" << std::endl;
    m_outputQueue->push(MessageType::MSG_UP, msg_up);
  }

  bool startSync() {
    std::cout << "start sync" << std::endl;
    m_block_synchronizer.startBlockSync(&endSync);
  }

  void endSync() {
    std::cout << "end sync" << std::endl;
    sendMsgUp();
    // TODO : BPscheduler, MessageFetcher 구동
  }
};
} // namespace gruut