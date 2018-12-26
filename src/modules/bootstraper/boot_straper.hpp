#pragma once

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/setting.hpp"
#include "../../utils/time.hpp"
#include "../module.hpp"
#include "block_synchronizer.hpp"
#include "nlohmann/json.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace gruut {

class BootStraper : public Module {
private:
  BlockSynchronizer m_block_synchronizer;
  merger_id_type m_my_id;
  local_chain_id_type m_my_localchain_id;
  MessageProxy m_msg_proxy;

public:
  BootStraper() {
    auto setting = Setting::getInstance();
    m_my_id = setting->getMyId();
    m_my_localchain_id = setting->getLocalChainId();
  }
  ~BootStraper() = default;

  void start() override { startSync(); }

private:
  void sendMsgUp() {

    cout << "BST: sendMsgUp()" << endl;

    OutputMsgEntry output_msg;
    output_msg.type = MessageType::MSG_UP;
    output_msg.body["mID"] = TypeConverter::toBase64Str(m_my_id);
    output_msg.body["time"] = Time::now();
    output_msg.body["ver"] = to_string(1);
    output_msg.body["cID"] = TypeConverter::toBase64Str(m_my_localchain_id);

    m_msg_proxy.deliverOutputMessage(output_msg);
  }

  void startSync() {

    cout << "BST: startSync()" << endl;

    m_block_synchronizer.startBlockSync(
        std::bind(&BootStraper::endSync, this, std::placeholders::_1));
  }

  void endSync(ExitCode exit_code) {

    cout << "BST: endSync(" << (int)exit_code << ")" << endl;

    if (exit_code == ExitCode::NORMAL ||
        exit_code == ExitCode::ERROR_SYNC_ALONE) { // complete done or alone
      sendMsgUp();

      // TODO : BPscheduler, MessageFetcher 구동

      stageOver(exit_code);

    } else {
      std::this_thread::sleep_for(
          std::chrono::seconds(config::BOOTSTRAP_RETRY_TIMEOUT));
      startSync();
    }
  }
};
} // namespace gruut