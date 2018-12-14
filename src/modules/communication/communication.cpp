#include "communication.hpp"
#include "../../config/config.hpp"
#include "merger_server.hpp"
#include <thread>

namespace gruut {
void Communication::startCommunicationLoop() {
  auto &io_service = Application::app().getIoService();
  io_service.post([]() {
    MergerServer server;
    server.runServer(config::PORT_NUM);
  });
}
} // namespace gruut
