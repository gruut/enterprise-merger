#ifndef GRUUT_ENTERPRISE_MERGER_BOOTSTRAPPER_HPP
#define GRUUT_ENTERPRISE_MERGER_BOOTSTRAPPER_HPP

#include "nlohmann/json.hpp"

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/setting.hpp"
#include "../../utils/time.hpp"
#include "../communication/communication.hpp"
#include "../module.hpp"

#include "block_synchronizer.hpp"

#include <boost/asio.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace gruut {

class Bootstrapper : public Module {
private:
  BlockSynchronizer m_block_synchronizer;
  merger_id_type m_my_id;
  localchain_id_type m_my_localchain_id;
  MessageProxy m_msg_proxy;
  std::shared_ptr<Communication> m_communication;
  std::once_flag m_endsync_flag;

public:
  Bootstrapper();

  void setCommunication(std::shared_ptr<Communication> communication) {
    m_communication = communication;
  }

  void start() override;

private:
  void sendMsgUp();
  void selfCheckUp();
  void endSync(ExitCode exit_code);
};
} // namespace gruut

#endif