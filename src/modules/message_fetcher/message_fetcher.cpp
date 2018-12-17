#include <chrono>
#include <iostream>
#include <thread>

#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/message_proxy.hpp"
#include "message_fetcher.hpp"

namespace gruut {
MessageFetcher::MessageFetcher() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
}

void MessageFetcher::start() { fetch(); }

void MessageFetcher::fetch() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    auto &input_queue = Application::app().getInputQueue();
    if (!input_queue->empty()) {
      auto input_message = input_queue->front();
      input_queue->pop();

      MessageProxy message_proxy;
      message_proxy.deliverInputMessage(input_message);
    }
  });

  m_timer->expires_from_now(
      boost::posix_time::milliseconds(config::INQUEUE_MSG_FETCHER_INTVAL));
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