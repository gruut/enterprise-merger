#include "out_message_fetcher.hpp"
#include "../../application.hpp"
#include "../communication/message_handler.hpp"

namespace gruut {

OutMessageFetcher::OutMessageFetcher() {
  m_output_queue = OutputQueueAlt::getInstance();
  m_fetch_scheduler.setIoService(Application::app().getIoService());
}

void OutMessageFetcher::start() {
  m_fetch_scheduler.setInterval(config::OUTQUEUE_MSG_FETCHER_INTERVAL);
  m_fetch_scheduler.setTaskFunction(std::bind(&OutMessageFetcher::fetch, this));
  m_fetch_scheduler.runTask();
}

void OutMessageFetcher::fetch() {
  while (!m_output_queue->empty()) {
    auto output_msg = m_output_queue->fetch();
    MessageHandler msg_handler;
    msg_handler.packMsg(output_msg);
  }
}

} // namespace gruut
