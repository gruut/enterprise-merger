#include "communication.hpp"
#include "../../application.hpp"
#include "../../config/config.hpp"
#include <thread>

namespace gruut {
Communication::Communication() {
  m_port_num = Setting::getInstance()->getMyPort();
  if (m_port_num.empty())
    m_port_num = config::DEFAULT_PORT_NUM;

  m_merger_client.setup();

  setUpConnList();
};

void Communication::start() {
  m_merger_client.accessToTracker();
  m_merger_client.checkRpcConnection();
  m_merger_client.checkHttpConnection();

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() { m_merger_server.runServer(m_port_num); });
}

void Communication::setUpConnList() {
  auto setting = Setting::getInstance();
  auto merger_list = setting->getMergerInfo();
  auto se_list = setting->getServiceEndpointInfo();

  auto tracker_info = setting->getTrackerInfo();

  merger_id_type my_id = setting->getMyId();

  auto conn_manager = ConnManager::getInstance();

  conn_manager->setTrackerInfo(tracker_info, true);

  for (auto &merger_info : merger_list) {
    if (merger_info.id == my_id)
      continue;

    conn_manager->setMergerInfo(merger_info, true);
  }
  for (auto &se_info : se_list) {
    conn_manager->setSeInfo(se_info, true);
  }
}
} // namespace gruut
