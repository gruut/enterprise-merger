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

  m_input_queue = InputQueueAlt::getInstance();
  m_fetch_scheduler.setIoService(Application::app().getIoService());;

  el::Loggers::getLogger("MFCT");
}

void MessageFetcher::start() {

  m_fetch_scheduler.setInterval(config::INQUEUE_MSG_FETCHER_INTERVAL);
  m_fetch_scheduler.setTaskFunction(std::bind(&MessageFetcher::fetch, this));
  m_fetch_scheduler.runTask();

}

void MessageFetcher::fetch() {
    std::vector<InputMsgEntry> input_messages = m_input_queue->fetchBulk();
    MessageProxy message_proxy;
    for (auto &msg : input_messages)
      message_proxy.deliverInputMessage(msg);
}
} // namespace gruut