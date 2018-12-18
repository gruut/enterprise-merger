#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FETCHER_HPP
#include "../../services/input_queue.hpp"
#include "../module.hpp"
#include <boost/asio.hpp>

namespace gruut {
class MessageFetcher : public Module {
public:
  MessageFetcher();

  void start() override;

private:
  void fetch();
  InputQueueAlt *m_input_queue;
  std::unique_ptr<boost::asio::deadline_timer> m_timer;
};
} // namespace gruut
#endif
