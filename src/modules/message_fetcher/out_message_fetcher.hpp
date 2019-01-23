#ifndef GRUUT_ENTERPRISE_MERGER_OUT_MESSAGE_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_OUT_MESSAGE_FETCHER_HPP

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/output_queue.hpp"
#include "../../utils/periodic_task.hpp"
#include "../module.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace gruut {
class OutMessageFetcher : public Module {
public:
  OutMessageFetcher();

  void start() override;

private:
  void fetch();
  OutputQueueAlt *m_output_queue;
  PeriodicTask m_fetch_scheduler;
};

} // namespace gruut

#endif