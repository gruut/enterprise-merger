#pragma once

#include "../../../include/nlohmann/json.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/output_queue.hpp"
#include "../../services/setting.hpp"
#include "../../utils/time.hpp"
#include "block_synchronizer.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

namespace gruut {

class BootStraper {
private:
  OutputQueueAlt *m_outputQueue;
  BlockSynchronizer m_block_synchronizer;
  Setting *m_setting;

  merger_id_type m_my_id;
  local_chain_id_type m_my_localchain_id;

public:
  BootStraper() {
    m_outputQueue = OutputQueueAlt::getInstance();
    m_setting = Setting::getInstance();

    m_my_id = m_setting->getMyId();
    m_my_localchain_id = m_setting->getLocalChainId();
  }
  ~BootStraper() {}

  void sendMsgUp() {

    nlohmann::json msg_up;
    msg_up["mID"] = TypeConverter::toBase64Str(m_my_id);
    msg_up["time"] = Time::now();
    msg_up["ver"] = to_string(1);
    msg_up["cID"] = TypeConverter::toBase64Str(m_my_localchain_id);

    m_outputQueue->push(MessageType::MSG_UP, msg_up);
  }

  void startSync() {
    m_block_synchronizer.setMyID(m_my_id);
    m_block_synchronizer.startBlockSync(
        std::bind(&BootStraper::endSync, this, std::placeholders::_1));
  }

  void endSync(int exit_code) {
    if (exit_code == 1) {
      sendMsgUp();

      // TODO : BPscheduler, MessageFetcher 구동

    } else {
      std::this_thread::sleep_for(
          std::chrono::seconds(config::BOOTSTRAP_RETRY_TIMEOUT));
      startSync();
    }
  }
};
} // namespace gruut