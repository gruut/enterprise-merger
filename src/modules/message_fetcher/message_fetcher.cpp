#include <chrono>
#include <iostream>
#include <thread>

#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/message_proxy.hpp"
#include "message_fetcher.hpp"

#include "easy_logging.hpp"

namespace gruut {
MessageFetcher::MessageFetcher() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_input_queue = InputQueueAlt::getInstance();

  el::Loggers::getLogger("MFCT");
}

void MessageFetcher::start() { fetch(); }

void MessageFetcher::fetch() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    std::vector<InputMsgEntry> input_messages = m_input_queue->fetchBulk();

    MessageProxy message_proxy;
    for (auto &msg : input_messages)
      message_proxy.deliverInputMessage(msg);
  });

  m_timer->expires_from_now(
      boost::posix_time::milliseconds(config::INQUEUE_MSG_FETCHER_INTERVAL));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "MFCT") << "Timer ABORTED";
    } else if (ec.value() == 0) {
      fetch();
    } else {
      CLOG(ERROR, "MFCT") << ec.message();
      // throw;
    }
  });
}
} // namespace gruut