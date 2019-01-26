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
  output_msg.body["ip"] = setting->getMyAddress();
  output_msg.body["port"] = setting->getMyPort();
  output_msg.body["mCert"] = setting->getMyCert();

  m_msg_proxy.deliverOutputMessage(output_msg);
}

void Bootstrapper::selfCheckUp() {
  CLOG(INFO, "BOOT") << "[1] Waiting server to start";
  while (!m_communication->isStarted()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  CLOG(INFO, "BOOT") << "[2] Waiting connection check";

  std::this_thread::sleep_for(std::chrono::seconds(config::MAX_WAIT_CONNECT_OTHERS_BSYNC_SEC));

  auto last_block_status = Application::app().getBlockProcessor().getMostPossibleLink();
  auto conn_manager = ConnManager::getInstance();
  auto max_height_mids = conn_manager->getMaxBlockHgtMergers();
  conn_manager->clearBlockHgtList();

  block_height_type max_block_height = max_height_mids.first;

  if(max_height_mids.second.empty()) {
    CLOG(INFO, "BOOT") << "[3] Starting block synchronization (not sure)";
    m_block_synchronizer.startBlockSync(std::bind(&Bootstrapper::endSync, this, std::placeholders::_1));
    return;
  } else {
    if(last_block_status.height >= max_block_height) {
      CLOG(INFO, "BOOT") << "[3] Skip block synchronization (no need)";
      endSync(ExitCode::NORMAL);
      return;
    }
  }

  CLOG(INFO, "BOOT") << "[3] Waiting connection to highest merger";

  bool conn_check = false;
  timestamp_t start_time = Time::now_int();

  while (!conn_check) {
    for (auto &merger_id : max_height_mids.second) {
      conn_check |= conn_manager->getMergerStatus(merger_id);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if ((Time::now_int() - start_time) > config::MAX_WAIT_CONNECT_OTHERS_BSYNC_SEC)
      break;
  }

  if(conn_check) {
    CLOG(INFO, "BOOT") << "[4] Starting block synchronization";
    m_block_synchronizer.startBlockSync(std::bind(&Bootstrapper::endSync, this, std::placeholders::_1));
  } else {
    CLOG(INFO, "BOOT") << "[4] Skip block synchronization (no connection)";
    endSync(ExitCode::ERROR_SYNC_ALONE);
  }
}

void Bootstrapper::endSync(ExitCode exit_code) {

  std::call_once(m_endsync_flag,[this, &exit_code](){
    sendMsgUp();
    stageOver(exit_code);
  });
}

} // namespace gruut
