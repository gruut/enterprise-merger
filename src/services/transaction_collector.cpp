#include <iostream>

#include "../application.hpp"
#include "../chain/transaction.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"
#include "transaction_collector.hpp"
#include <botan/base64.h>
#include <botan/data_src.h>
#include <botan/x509_key.h>

using namespace std;
using namespace nlohmann;

namespace gruut {
void TransactionCollector::handleMessage(json message_body_json) {
  if (isRunnable()) {
    if (!m_timer_running) {
      m_timer_running = true;
      startTimer();
      Application::app().getSignerPool().createTransactions();
      m_signature_requester.requestSignatures();
    }

    Transaction transaction;
    bytes signature_message;

    auto txid_vector =
        Botan::base64_decode(message_body_json["txid"].get<string>());
    transaction.transaction_id = TypeConverter::toBytes(
        string(txid_vector.cbegin(), txid_vector.cend()));
    signature_message.insert(signature_message.cend(), txid_vector.cbegin(),
                             txid_vector.cend());

    transaction.sent_time =
        TypeConverter::to8BytesArray(message_body_json["time"].get<string>());
    signature_message.insert(signature_message.cend(),
                             transaction.sent_time.cbegin(),
                             transaction.sent_time.cend());

    auto requestor_id_vector =
        Botan::base64_decode(message_body_json["rID"].get<string>());
    transaction.requestor_id = TypeConverter::toBytes(
        string(requestor_id_vector.cbegin(), requestor_id_vector.cend()));
    signature_message.insert(signature_message.cend(),
                             requestor_id_vector.cbegin(),
                             requestor_id_vector.cend());

    string transaction_type_string = message_body_json["type"].get<string>();
    if (transaction_type_string == "digests")
      transaction.transaction_type = TransactionType::DIGESTS;
    else
      transaction.transaction_type = TransactionType::CERTIFICATE;
    auto transaction_type_bytes =
        TypeConverter::toBytes(transaction_type_string);
    signature_message.insert(signature_message.cend(),
                             transaction_type_bytes.cbegin(),
                             transaction_type_bytes.cend());

    json content_array_json = message_body_json["content"];
    for (auto it = content_array_json.cbegin(); it != content_array_json.cend();
         ++it) {
      string elem = (*it).get<string>();
      auto elem_bytes = TypeConverter::toBytes(elem);
      signature_message.insert(signature_message.cend(), elem_bytes.cbegin(),
                               elem_bytes.cend());

      transaction.content_list.emplace_back(elem);
    }

    auto rsig_vector =
        Botan::base64_decode(message_body_json["rSig"].get<string>());
    transaction.signature =
        vector<uint8_t>(rsig_vector.cbegin(), rsig_vector.cend());

    // TODO: Service endpoint로부터 public_key를 받을 수 있을 때 63-71줄 제거할
    // 것.
    string endpoint_public_key =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDCtTEic76GBqUetJ1XXrrWZcxd\n"
        "8vJr2raWRqBjbGpSzLqa3YLvVxVeK49iSlI+5uLX/2WFJdhKAWoqO+03oH4TDSup\n"
        "olzZrwMFSylxGwR5jPmoNHDMS3nnzUkBtdr3NCfq1C34fQV0iUGdlPtJaiiTBQPM\n"
        "t4KUcQ1TaazB8TzhqwIDAQAB\n"
        "-----END PUBLIC KEY-----";
    Botan::DataSource_Memory pk_datasource(endpoint_public_key);
    unique_ptr<Botan::Public_Key> public_key(
        Botan::X509::load_key(pk_datasource));

    bool is_verified = RSA::doVerify(*public_key, signature_message,
                                     transaction.signature, true);

    if (is_verified) {
      auto &transaction_pool = Application::app().getTransactionPool();
      transaction_pool.push(transaction);
    }
  }
}

bool TransactionCollector::isRunnable() {
  // TOOD: 항상 TransactionCollector가 동작하는 것은 아니다. 스케쥴러에 의해
  // 동작이 중단될 수도 있고, 이미 블럭 생성중이면 중단시켜야 한다.
  return true;
}

void TransactionCollector::startTimer() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_timer->expires_from_now(
      boost::posix_time::seconds(TRANSACTION_COLLECTION_INTERVAL_SEC));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      cout << "startTimer: Timer was cancelled or retriggered." << endl;
      this->m_timer_running = false;
    } else if (ec.value() == 0) {
      this->m_timer_running = false;
      // TODO: Logger
      cout << "Transaction POOL SIZE: "
           << Application::app().getTransactionPool().size() << endl;
    } else {
      this->m_timer_running = false;
      throw;
    }
  });
}
} // namespace gruut