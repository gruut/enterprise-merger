#include "communication.hpp"
#include "grpc_merger.hpp"
#include <thread>

namespace gruut {
void Communication::startCommunicationLoop() {
  auto &io_service = Application::app().getIoService();
  io_service.post([]() {
    MergerRpcServer server;
    const char *port = "50051";
    server.runSignerServ(port);
  });
}
} // namespace gruut
