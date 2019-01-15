#include "bootstrapper.hpp"
#include "../../application.hpp"
#include "../../services/setting.hpp"
#include "easy_logging.hpp"

namespace gruut {

Bootstrapper::Bootstrapper() {
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  m_my_localchain_id = setting->getLocalChainId();
  el::Loggers::getLogger("BOOT");
}

void Bootstrapper::start() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() { selfCheckUp(); });
}

void Bootstrapper::sendMsgUp() {
  CLOG(INFO, "BOOT") << "send MSG_UP";
  auto setting = Setting::getInstance();
  OutputMsgEntry output_msg;
  // TODO : Message에 항목은 변경 될 수 있습니다.
  output_msg.type = MessageType::MSG_UP;
  output_msg.body["mID"] = TypeConverter::encodeBase64(m_my_id);
  output_msg.body["time"] = Time::now();
  output_msg.body["ver"] = to_string(1);
  output_msg.body["cID"] = TypeConverter::encodeBase64(m_my_localchain_id);
  output_msg.body["ip"] = setting->getMyId();
  output_msg.body["port"] = setting->getMyPort();
  output_msg.body["mCert"] = setting->getMyCert();

  m_msg_proxy.deliverOutputMessage(output_msg);
}

void Bootstrapper::selfCheckUp() {
  CLOG(INFO, "BOOT") << "1) Waiting server to start";
  while (!m_communication->isStarted()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  CLOG(INFO, "BOOT") << "2) Waiting connection check (in 5 sec)";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // TODO :: do jobs for self check-up

  CLOG(INFO, "BOOT") << "3) Starting block synchronization";

  m_block_synchronizer.startBlockSync(
      std::bind(&Bootstrapper::endSync, this, std::placeholders::_1));
}

void Bootstrapper::endSync(ExitCode exit_code) {
  /*
  if (exit_code == ExitCode::NORMAL ||
      exit_code == ExitCode::ERROR_SYNC_ALONE) { // complete done or alone
    sendMsgUp();

    stageOver(exit_code);

  } else {
    std::this_thread::sleep_for(
        std::chrono::seconds(config::BOOTSTRAP_RETRY_TIMEOUT));
    startSync();
  }
  */

  // TODO : rewrite these codes after fixing boot-hang bug
  sendMsgUp();
  stageOver(exit_code);
}

} // namespace gruut
