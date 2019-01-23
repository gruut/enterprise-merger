#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP
#include "../../services/input_queue.hpp"
#include "../module.hpp"
#include "../../utils/periodic_task.hpp"

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
