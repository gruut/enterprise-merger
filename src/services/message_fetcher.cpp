#include "message_fetcher.hpp"
#include "../application.hpp"

namespace gruut {
void MessageFetcher::fetch() {
  auto &input_queue = Application::app().getInputQueue();
  if (!input_queue->empty()) {
    auto input_message = input_queue->front();
    input_queue->pop();
  }
}
}
