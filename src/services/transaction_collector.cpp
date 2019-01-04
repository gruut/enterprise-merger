#include "transaction_collector.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

using namespace std;

namespace gruut {
TransactionCollector::TransactionCollector() {

  el::Loggers::getLogger("TXCO");

  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
}

void TransactionCollector::handleMessage(json &msg_body_json) {
  if (!isRunnable()) {
    CLOG(ERROR, "TXCO") << "TX dropped (not timing)";
    return;
  }

  auto new_txid = TypeConverter::base64ToArray<TRANSACTION_ID_TYPE_SIZE>(
      Safe::getString(msg_body_json, "txid"));

  auto &transaction_pool = Application::app().getTransactionPool();

  if (transaction_pool.isDuplicated(new_txid)) {
    CLOG(ERROR, "TXCO") << "TX dropped (duplicated)";
    return;
  }

  Transaction new_tx;
  new_tx.setJson(msg_body_json);

  if (new_tx.isValid()) {
    transaction_pool.push(new_tx);
  } else {
    CLOG(ERROR, "TXCO") << "TX dropped (invalid)";
  }
}

bool TransactionCollector::isRunnable() {
  return (m_current_tx_status == BpStatus::PRIMARY ||
          m_current_tx_status == BpStatus::SECONDARY);
}

void TransactionCollector::setTxCollectStatus(BpStatus stat) {
  m_next_tx_status = stat;
  if (!m_timer_running) {
    turnOnTimer();
  }

  CLOG(INFO, "TXCO") << "Set status (" << (int)stat << ")";

  if (m_current_tx_status == BpStatus::PRIMARY) {
    if (m_next_tx_status == BpStatus::PRIMARY) {
      m_bpjob_sequence[1] = BpJobStatus::DO;
    } else if (m_next_tx_status == BpStatus::SECONDARY) {
      m_bpjob_sequence[1] = BpJobStatus::DONT;
      m_bpjob_sequence[2] = BpJobStatus::DO;
    } else {
      m_bpjob_sequence[1] = BpJobStatus::DONT;
    }
  } else if (m_current_tx_status == BpStatus::SECONDARY) {
    if (m_next_tx_status == BpStatus::PRIMARY) {
      m_bpjob_sequence[1] = BpJobStatus::DO;
    }
  } else {
    if (m_next_tx_status ==
        BpStatus::PRIMARY) { // the case when only 1 merger exists in network
      m_bpjob_sequence[1] = BpJobStatus::DO;
    } else if (m_next_tx_status == BpStatus::SECONDARY) {
      m_bpjob_sequence[1] = BpJobStatus::DONT;
      m_bpjob_sequence[2] = BpJobStatus::DO;
    }
  }
}

void TransactionCollector::turnOnTimer() {

  // CLOG(INFO, "TXCO") << "called turnOnTimer()";

  m_timer_running = true;

  m_bpjob_sequence.push_back(BpJobStatus::DONT);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);

  updateStatus();
}

void TransactionCollector::updateStatus() {

  // CLOG(INFO, "TXCO") << "called updateStatus()";

  size_t current_slot = Time::now_int() / BP_INTERVAL;
  time_t next_slot_begin = (current_slot + 1) * BP_INTERVAL;

  boost::posix_time::ptime task_time =
      boost::posix_time::from_time_t(next_slot_begin);

  m_timer->expires_at(task_time);
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
    } else if (ec.value() == 0) {

      postJob();
      updateStatus();

    } else {
      throw;
    }
  });
}

void TransactionCollector::postJob() {
  auto &io_service = Application::app().getIoService();

  io_service.post([this]() {
    m_current_tx_status = m_next_tx_status;

    BpJobStatus this_job = m_bpjob_sequence.front();
    m_bpjob_sequence.pop_front();
    m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
    if (this_job == BpJobStatus::DO &&
        Application::app().getTransactionPool().size() > 0) {
      Application::app().getSignerPool().createTransactions();
      m_signature_requester.requestSignatures();
    }
  });
}
} // namespace gruut