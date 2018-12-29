#include "boot_straper.hpp"
#include "../../application.hpp"

#include "easy_logging.hpp"

namespace gruut {

BootStraper::BootStraper() {
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  m_my_localchain_id = setting->getLocalChainId();
  el::Loggers::getLogger("BOOT");
}

void BootStraper::start() { selfCheckUp(); }

void BootStraper::sendMsgUp() {

  CLOG(INFO, "BOOT") << "send MSG_UP";

  OutputMsgEntry output_msg;
  output_msg.type = MessageType::MSG_UP;
  output_msg.body["mID"] = TypeConverter::encodeBase64(m_my_id);
  output_msg.body["time"] = Time::now();
  output_msg.body["ver"] = to_string(1);
  output_msg.body["cID"] = TypeConverter::encodeBase64(m_my_localchain_id);

  m_msg_proxy.deliverOutputMessage(output_msg);
}

void BootStraper::selfCheckUp() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    CLOG(INFO, "BOOT") << "Start Self Check-up";

    // TODO :: do jobs for self check-up

    CLOG(INFO, "BOOT") << "Waiting communication to start";
    while (!m_communication->isStarted()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    CLOG(INFO, "BOOT") << "Ended Self Check-up";

    startSync();
  });
}

void BootStraper::startSync() {

  CLOG(INFO, "BOOT") << "Start block synchronization";

  m_block_synchronizer.startBlockSync(
      std::bind(&BootStraper::endSync, this, std::placeholders::_1));
}

void BootStraper::endSync(ExitCode exit_code) {

  CLOG(INFO, "BOOT") << "Ended block synchronization (" << (int)exit_code
                     << ")";

  if (exit_code == ExitCode::NORMAL ||
      exit_code == ExitCode::ERROR_SYNC_ALONE) { // complete done or alone
    sendMsgUp();

    stageOver(exit_code);

  } else {
    std::this_thread::sleep_for(
        std::chrono::seconds(config::BOOTSTRAP_RETRY_TIMEOUT));
    startSync();
  }
}

} // namespace gruut
