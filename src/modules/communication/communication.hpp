#ifndef GRUUT_ENTERPRISE_MERGER_COMMUNICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_COMMUNICATION_HPP

#include "../communication/merger_client.hpp"
#include "../communication/merger_server.hpp"
#include "../module.hpp"

namespace gruut {

class Communication : public Module {
public:
  Communication();
  void start() override;

  inline bool isStarted() { return m_merger_server.isStarted(); }

private:
  void setUpConnList();

  MergerServer m_merger_server;
  MergerClient m_merger_client;
  std::string m_port_num;
};
} // namespace gruut

#endif
