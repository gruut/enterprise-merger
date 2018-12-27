#pragma once

#include "../communication/merger_client.hpp"
#include "../communication/merger_server.hpp"
#include "../module.hpp"

namespace gruut {

class Communication : public Module {
public:
  Communication();
  void start() override;

private:
  MergerServer m_merger_server;
  MergerClient m_merger_client;
  std::string m_port_num;
};
} // namespace gruut
