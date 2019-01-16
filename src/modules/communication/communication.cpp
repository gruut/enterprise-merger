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
};

void Communication::start() {
  //TODO : Tracker에 접속하지 못하였을때 처리 필요.
  m_merger_client.accessToTracker();
  m_merger_client.checkRpcConnection();
  m_merger_client.checkHttpConnection();

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() { m_merger_server.runServer(m_port_num); });
}
} // namespace gruut
