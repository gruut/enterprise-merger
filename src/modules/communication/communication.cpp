#include "communication.hpp"
#include "../../config/config.hpp"
#include "merger_server.hpp"
#include <thread>

namespace gruut {
void Communication::startCommunicationLoop() {
  auto &io_service = Application::app().getIoService();
  io_service.post([]() {
    MergerServer server;

    std::string port_num = Setting::getInstance()->getMyPort();
    if (port_num.empty())
      port_num = config::DEFAULT_PORT_NUM;

    server.runServer(port_num);
  });
}
} // namespace gruut
