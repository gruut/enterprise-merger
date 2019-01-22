#include "transaction_collector.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

using namespace std;

namespace gruut {
TransactionCollector::TransactionCollector() {

  el::Loggers::getLogger("TXCO");

  m_storage = Storage::getInstance();

  auto& io_service = Application::app().getIoService();
  m_timer.reset(new boost::asio::deadline_timer(io_service));
  m_postjob_strand.reset(new boost::asio::strand(io_service));
}

void TransactionCollector::handleMessage(json &msg_body_json) {
  if (!isRunnable()) {
    // CLOG(ERROR, "TXCO") << "TX dropped (not timing)";
    return;
  }

  std::string txid_b64 = Safe::getString(msg_body_json, "txid");

  auto new_txid =
      TypeConverter::base64ToArray<TRANSACTION_ID_TYPE_SIZE>(txid_b64);

  auto &transaction_pool = Application::app().getTransactionPool();

  if (transaction_pool.isDuplicated(new_txid)) {
    CLOG(ERROR, "TXCO") << "TX dropped (duplicated in pool)";
    return;
  }

  if (m_storage->isDuplicatedTx(txid_b64)) {
    CLOG(ERROR, "TXCO") << "TX dropped (duplicated in storage)";
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
  turnOnTimer();

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
  std::call_once(m_timer_once_flag, [this](){

    m_bpjob_sequence.push_back(BpJobStatus::DONT);
    m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
    m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);

    updateStatus();
  });
}

void TransactionCollector::updateStatus() {

  size_t current_time = Time::now_ms();
  size_t current_slot = current_time / (BP_INTERVAL * 1000);
  timestamp_type next_slot_begin = (current_slot + 1) * (BP_INTERVAL * 1000);
  timestamp_type time_to_next = next_slot_begin - current_time;

  if(time_to_next < 1000) // may be system time rewind
    time_to_next = BP_INTERVAL * 1000 - time_to_next;

  m_timer->expires_from_now(boost::posix_time::milliseconds(time_to_next));
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

  io_service.post(m_postjob_strand->wrap([this]() {
    m_current_tx_status = m_next_tx_status;

    BpJobStatus this_job = m_bpjob_sequence.front();
    m_bpjob_sequence.pop_front();
    m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
    if (this_job == BpJobStatus::DO &&
        Application::app().getTransactionPool().size() > 0) {
      m_signature_requester.requestSignatures();
    }
  }));
}

} // namespace gruut