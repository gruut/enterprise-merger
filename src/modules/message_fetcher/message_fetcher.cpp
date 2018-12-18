#include <chrono>
#include <iostream>
#include <thread>

#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../services/message_proxy.hpp"
#include "message_fetcher.hpp"

namespace gruut {
MessageFetcher::MessageFetcher() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_input_queue = InputQueueAlt::getInstance();
}

void MessageFetcher::start() { fetch(); }

void MessageFetcher::fetch() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    if (!m_input_queue->empty()) {
      auto input_message = m_input_queue->fetch();

      MessageProxy message_proxy;
      message_proxy.deliverInputMessage(input_message);
    }
  });

  // TODO: 임시로 1000(1초)
  m_timer->expires_from_now(boost::posix_time::milliseconds(1000));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      std::cout << "MessageFetcher: Timer was cancelled or retriggered."
                << std::endl;
    } else if (ec.value() == 0) {
      fetch();
    } else {
      std::cout << "ERROR: " << ec.message() << std::endl;
      throw;
    }
  });
}
} // namespace gruut