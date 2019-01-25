#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/message_proxy.hpp"
#include "../../utils/periodic_task.hpp"

#include "../module.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace gruut {
class MessageFetcher : public Module {
public:
  MessageFetcher();

  void start() override;

private:
  void fetch();
  InputQueueAlt *m_input_queue;
  PeriodicTask m_fetch_scheduler;
};
} // namespace gruut
#endif
