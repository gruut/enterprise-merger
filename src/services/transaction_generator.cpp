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
void TransactionGenerator::generate(Signer &signer) {
  if (signer.isNew()) {
    Transaction new_transaction;
    bytes signature_message;

    new_transaction.transaction_id = generateTransactionId();
    signature_message.insert(signature_message.cend(),
                             new_transaction.transaction_id.cbegin(),
                             new_transaction.transaction_id.cend());

    string timestamp = Time::now();
    new_transaction.sent_time = TypeConverter::digitStringToBytes(timestamp);
    signature_message.insert(signature_message.cend(),
                             new_transaction.sent_time.cbegin(),
                             new_transaction.sent_time.cend());

    // TODO: requestor_id <- Merger Id, 임시로 txID
    new_transaction.requestor_id = requestor_id_type();
    signature_message.insert(signature_message.cend(),
                             new_transaction.requestor_id.cbegin(),
                             new_transaction.requestor_id.cend());

    new_transaction.transaction_type = TransactionType::CERTIFICATE;
    string transaction_type_str = "certificate";
    signature_message.insert(signature_message.cend(),
                             transaction_type_str.cbegin(),
                             transaction_type_str.cend());

    auto user_id_str = to_string(signer.user_id);
    new_transaction.content_list.emplace_back(user_id_str);
    signature_message.insert(signature_message.cend(), user_id_str.cbegin(),
                             user_id_str.cend());

    new_transaction.content_list.emplace_back(signer.pk_cert);
    signature_message.insert(signature_message.cend(), signer.pk_cert.cbegin(),
                             signer.pk_cert.cend());

    auto &setting = Application::app().getSetting();
    string rsa_sk_pem = setting.getMySK();
    string rsa_sk_pass = setting.getMyPass();

    new_transaction.signature =
        RSA::doSign(rsa_sk_pem, signature_message, true, rsa_sk_pass);

    Application::app().getTransactionPool().push(new_transaction);
  }
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