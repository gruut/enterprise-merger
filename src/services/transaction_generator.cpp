#include "transaction_generator.hpp"
#include "../application.hpp"
#include "../chain/transaction.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"
#include <climits>
#include <random>

namespace gruut {
void TransactionGenerator::generate(vector<Signer> &signers) {
  if (signers.empty()) {
    return;
  }

  Transaction new_transaction;
  bytes signature_message;
  BytesBuilder msg_builder;

  Setting *setting = Setting::getInstance();
  string rsa_sk_pem = setting->getMySK();
  string rsa_sk_pass = setting->getMyPass();

  new_transaction.transaction_id = generateTransactionId();

  auto timestamp = (timestamp_type)Time::now_int();
  new_transaction.sent_time = timestamp;
  new_transaction.requestor_id = setting->getMyId();
  new_transaction.transaction_type = TransactionType::CERTIFICATE;

  msg_builder.append(new_transaction.transaction_id);
  msg_builder.append(timestamp);
  msg_builder.append(new_transaction.requestor_id);
  msg_builder.append(TXTYPE_CERTIFICATES);

  for (auto &signer : signers) {
    if (signer.isNew()) {

      auto user_id_str = TypeConverter::toBase64Str(signer.user_id);
      new_transaction.content_list.emplace_back(user_id_str);
      new_transaction.content_list.emplace_back(signer.pk_cert);

      msg_builder.append(user_id_str);
      msg_builder.append(signer.pk_cert);
    }
  }

  new_transaction.signature =
      RSA::doSign(rsa_sk_pem, msg_builder.getBytes(), true, rsa_sk_pass);

  Application::app().getTransactionPool().push(new_transaction);
}

transaction_id_type TransactionGenerator::generateTransactionId() {
  mt19937 mt;
  mt.seed(random_device()());

  std::uniform_int_distribution<std::mt19937::result_type> dist;
  auto random_number = dist(mt);

  auto bytes = TypeConverter::integerToBytes(random_number);
  transaction_id_type tx_id =
      TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(bytes);

  return tx_id;
}
} // namespace gruut