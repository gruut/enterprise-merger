#include "transaction_collector.hpp"
#include "../application.hpp"
#include "../chain/transaction.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"
#include <boost/assert.hpp>
#include <botan-2/botan/base64.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/x509_key.h>
#include <iostream>

using namespace std;
using namespace nlohmann;

namespace gruut {
TransactionCollector::TransactionCollector() {
  m_service_endpoints = Setting::getInstance()->getServiceEndpointInfo();
}

void TransactionCollector::handleMessage(json msg_body_json) {
  if (!isRunnable())
    return;

  Transaction transaction;
  BytesBuilder sig_msg_builder;

  string txid_b64 = msg_body_json["txid"].get<string>();
  auto txid_bytes = TypeConverter::decodeBase64(txid_b64);
  BOOST_ASSERT_MSG(txid_bytes.size() == 32,
                   "The size of the transaction is not 32 bytes");
  transaction.transaction_id =
      TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(txid_bytes);
  sig_msg_builder.append(txid_bytes);

  string time_str = msg_body_json["time"].get<string>();
  auto sent_time = static_cast<timestamp_type>(stoll(time_str));
  transaction.sent_time = sent_time;
  sig_msg_builder.append(sent_time);

  string rid_b64 = msg_body_json["rID"].get<string>();
  auto rid_bytes = TypeConverter::decodeBase64(rid_b64);
  transaction.requestor_id = static_cast<id_type>(rid_bytes);
  sig_msg_builder.append(rid_bytes);

  string tx_type_str = msg_body_json["type"].get<string>();
  if (tx_type_str == TXTYPE_DIGESTS)
    transaction.transaction_type = TransactionType::DIGESTS;
  else
    transaction.transaction_type = TransactionType::CERTIFICATE;
  sig_msg_builder.append(tx_type_str);

  if (msg_body_json["content"].is_array()) {
    for (auto &cont_item : msg_body_json["content"]) {
      string cont_str = cont_item.get<string>();
      transaction.content_list.emplace_back(cont_str);
      sig_msg_builder.append(cont_str);
    }
  }

  auto rsig_b64 = msg_body_json["rSig"].get<string>();
  auto rsig_bytes = TypeConverter::decodeBase64(rsig_b64);
  transaction.signature = rsig_bytes;

  string endpoint_cert;

  for (auto &srv_point : m_service_endpoints) {
    if (srv_point.id == rid_bytes) {
      endpoint_cert = srv_point.cert;
    }
  }

  if (endpoint_cert.empty())
    return;

  if (!RSA::doVerify(endpoint_cert, sig_msg_builder.getBytes(), rsig_bytes,
                     true))
    return;

  Application::app().getTransactionPool().push(transaction);
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

  cout << "TXC: setTxCollectStatus(" << (int)stat << ")" << endl << flush;

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

  cout << "TXC: turnOnTimer()" << endl;

  m_timer_running = true;

  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));

  m_bpjob_sequence.push_back(BpJobStatus::DONT);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);

  updateStatus();
}

void TransactionCollector::updateStatus() {

  cout << "TXC: updateStatus()" << endl;

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