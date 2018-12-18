#pragma once

#include "../module.hpp"
#include "../../services/output_queue.hpp"
#include <boost/asio.hpp>

namespace gruut {
class OutMessageFetcher : public Module {
public:
  OutMessageFetcher();

  void start() override;

private:
  void fetch();
  OutputQueueAlt *m_output_queue;
  std::unique_ptr<boost::asio::deadline_timer> m_timer;
};

} // namespace gruut