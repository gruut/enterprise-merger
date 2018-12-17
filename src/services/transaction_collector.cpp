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
void TransactionCollector::handleMessage(json message_body_json) {
  if (!isRunnable())
    return;

  Transaction transaction;
  BytesBuilder bytes_builder;

  string txid_str = message_body_json["txid"].get<string>();
  auto txid_bytes = TypeConverter::decodeBase64(txid_str);
  BOOST_ASSERT_MSG(txid_bytes.size() == 32,
                   "The size of the transaction is not 32 bytes");
  transaction.transaction_id =
      TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(txid_bytes);
  bytes_builder.append(txid_bytes);

  string t_str = message_body_json["time"].get<string>();
  auto sent_time = static_cast<timestamp_type>(stoll(t_str));
  transaction.sent_time = sent_time;
  bytes_builder.append(sent_time);

  string r_id_str = message_body_json["rID"].get<string>();
  auto requestor_id_vector = TypeConverter::decodeBase64(r_id_str);
  transaction.requestor_id = requestor_id_vector;
  bytes_builder.append(requestor_id_vector);

  string transaction_type_string = message_body_json["type"].get<string>();
  if (transaction_type_string == TXTYPE_DIGESTS)
    transaction.transaction_type = TransactionType::DIGESTS;
  else
    transaction.transaction_type = TransactionType::CERTIFICATE;
  auto transaction_type_bytes =
      TypeConverter::stringToBytes(transaction_type_string);
  bytes_builder.append(transaction_type_bytes);

  json content_array_json = message_body_json["content"];
  for (auto it = content_array_json.cbegin(); it != content_array_json.cend();
       ++it) {
    string elem = (*it).get<string>();
    auto elem_bytes = TypeConverter::stringToBytes(elem);
    bytes_builder.append(elem_bytes);

    transaction.content_list.emplace_back(elem);
  }

  auto rsig_vector =
      Botan::base64_decode(message_body_json["rSig"].get<string>());
  transaction.signature =
      vector<uint8_t>(rsig_vector.cbegin(), rsig_vector.cend());

  Setting *setting = Setting::getInstance();

  std::vector<ServiceEndpointInfo> servend_info =
      setting->getServiceEndpointInfo();

  string endpoint_cert;

  for (auto &item : servend_info) {
    if (item.id == requestor_id_vector) {
      endpoint_cert = item.cert;
    }
  }

  if (endpoint_cert.empty())
    return;

  auto signature_message_bytes = bytes_builder.getBytes();
  bool is_verified = RSA::doVerify(endpoint_cert, signature_message_bytes,
                                   transaction.signature, true);

  if (is_verified) {
    auto &transaction_pool = Application::app().getTransactionPool();
    transaction_pool.push(transaction);
  }
}

bool TransactionCollector::isRunnable() {
  return (m_current_tx_status == BpStatus::PRIMARY || m_current_tx_status == BpStatus::SECONDARY);
}

void TransactionCollector::setTxCollectStatus(BpStatus stat){
  m_next_tx_status = stat;
  if(!m_timer_running) {
    turnOnTimer();
  }

  if(m_current_tx_status == BpStatus::PRIMARY) {
    if(m_next_tx_status == BpStatus::PRIMARY) {
      m_bpjob_sequence[1] = BpJobStatus::DO;
    }
    else if(m_next_tx_status == BpStatus::SECONDARY){
      m_bpjob_sequence[1] = BpJobStatus::DONT;
      m_bpjob_sequence[2] = BpJobStatus::DO;
    }
    else {
      m_bpjob_sequence[1] = BpJobStatus::DONT;
    }
  }
  else if(m_current_tx_status == BpStatus::SECONDARY) {
    if(m_next_tx_status == BpStatus::PRIMARY) {
      m_bpjob_sequence[1] = BpJobStatus::DO;
    }
  }
  else {
    if(m_next_tx_status == BpStatus::PRIMARY) { // the case when only 1 merger exists in network
      m_bpjob_sequence[1] = BpJobStatus::DO;
    }
    else if(m_next_tx_status == BpStatus::SECONDARY) {
      m_bpjob_sequence[1] = BpJobStatus::DONT;
      m_bpjob_sequence[2] = BpJobStatus::DO;
    }
  }

}

void TransactionCollector::turnOnTimer() {

  m_timer_running = true;

  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));

  m_bpjob_sequence.push_back(BpJobStatus::DONT);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
  m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);

  updateStatus();
}

void TransactionCollector::updateStatus() {

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

void TransactionCollector::postJob(){
  auto &io_service = Application::app().getIoService();

  io_service.post([&](){

    m_current_tx_status = m_next_tx_status;

    BpJobStatus this_job = m_bpjob_sequence.front();
    m_bpjob_sequence.pop_front();
    m_bpjob_sequence.push_back(BpJobStatus::UNKNOWN);
    if(this_job == BpJobStatus::DO) {
      cout << "Transaction POOL SIZE: "
           << Application::app().getTransactionPool().size() << endl;

      Application::app().getSignerPool().createTransactions();
      m_signature_requester.requestSignatures();
    }
  });
}
} // namespace gruut