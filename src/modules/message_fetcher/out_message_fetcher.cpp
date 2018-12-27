#include "out_message_fetcher.hpp"
#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../communication/message_handler.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace gruut {

OutMessageFetcher::OutMessageFetcher() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_output_queue = OutputQueueAlt::getInstance();
}

void OutMessageFetcher::start() { fetch(); }

void OutMessageFetcher::fetch() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    while (!m_output_queue->empty()) {
      auto output_msg = m_output_queue->fetch();

      MessageHandler msg_handler;
      msg_handler.packMsg(output_msg);
    }
  });

  m_timer->expires_from_now(
      boost::posix_time::milliseconds(config::OUTQUEUE_MSG_FETCHER_INTERVAL));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      std::cout << "Out MessageFetcher: Timer was cancelled or retriggered."
                << std::endl;
    } else if (ec.value() == 0) {
      fetch();
    } else {
      std::cout << "ERROR: " << ec.message() << std::endl;
      // throw;
    }
  });
}

} // namespace gruut
