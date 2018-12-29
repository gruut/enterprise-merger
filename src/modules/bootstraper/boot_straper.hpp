#pragma once

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/setting.hpp"
#include "../../utils/time.hpp"
#include "../communication/communication.hpp"
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
  std::shared_ptr<Communication> m_communication;

public:
  BootStraper();

  inline void setCommunication(std::shared_ptr<Communication> communication) {
    m_communication = communication;
  }

  void start() override;

private:
  void sendMsgUp();
  void selfCheckUp();
  void startSync();
  void endSync(ExitCode exit_code);
};
} // namespace gruut