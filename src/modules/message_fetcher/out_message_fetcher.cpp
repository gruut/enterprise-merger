#include "out_message_fetcher.hpp"
#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../communication/message_handler.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace gruut {

OutMessageFetcher::OutMessageFetcher() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
}

void OutMessageFetcher::start() { fetch(); }

void OutMessageFetcher::fetch() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    auto &output_queue = Application::app().getOutputQueue();
    while (!output_queue->empty()) {
      OutputMessage output_msg = output_queue->front();
      output_queue->pop();

      MessageHandler msg_handler;
      msg_handler.packMsg(output_msg);
    }
  });

  m_timer->expires_from_now(boost::posix_time::milliseconds(1000));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      std::cout << "Out MessageFetcher: Timer was cancelled or retriggered."
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
