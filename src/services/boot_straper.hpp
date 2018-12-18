#pragma once

#include "nlohmann/json.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "output_queue.hpp"
#include "setting.hpp"
#include "../utils/time.hpp"
#include "../modules/bootstraper/block_synchronizer.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <memory>

namespace gruut {

class BootStraper {
private:
  BlockSynchronizer m_block_synchronizer;
  merger_id_type m_my_id;
  local_chain_id_type m_my_localchain_id;
  std::function<void(int)> m_finish_callback;

public:
  BootStraper() {
    auto setting = Setting::getInstance();
    m_my_id = setting->getMyId();
    m_my_localchain_id = setting->getLocalChainId();
  }
  ~BootStraper() = default;

  void sendMsgUp() {

    cout << "BST: sendMsgUp()" << endl;

    nlohmann::json msg_up;
    msg_up["mID"] = TypeConverter::toBase64Str(m_my_id);
    msg_up["time"] = Time::now();
    msg_up["ver"] = to_string(1);
    msg_up["cID"] = TypeConverter::toBase64Str(m_my_localchain_id);


    auto outputQueue = OutputQueueAlt::getInstance();
    outputQueue->push(MessageType::MSG_UP, msg_up);
  }

  void startSync(std::function<void(int)> callback) {

    cout << "BST: startSync()" << endl;

    m_finish_callback = std::move(callback);
    m_block_synchronizer.setMyID(m_my_id);
    m_block_synchronizer.startBlockSync(
        std::bind(&BootStraper::endSync, this, std::placeholders::_1));
  }

  void endSync(int exit_code) {

    cout << "BST: endSync(" << exit_code << ")" << endl;

    if (exit_code == 1) {
      sendMsgUp();

      // TODO : BPscheduler, MessageFetcher 구동

      m_finish_callback(1);

    } else {
      std::this_thread::sleep_for(
          std::chrono::seconds(config::BOOTSTRAP_RETRY_TIMEOUT));
      startSync(m_finish_callback);
    }
  }
};
} // namespace gruut