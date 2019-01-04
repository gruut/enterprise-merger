#include "transaction_generator.hpp"
#include "../application.hpp"

namespace gruut {
void TransactionGenerator::generate(vector<Signer> &signers) {
  if (signers.empty()) {
    return;
  }

  Transaction new_transaction;

  auto setting = Setting::getInstance();

  new_transaction.setId(generateTransactionId());
  new_transaction.setTime(static_cast<timestamp_type>(Time::now_int()));
  new_transaction.setRequestorId(setting->getMyId());
  new_transaction.setTransactionType(TransactionType::CERTIFICATE);

  std::vector<content_type> content_list;
  for (auto &signer : signers) {
    if (signer.isNew()) {
      auto user_id_str = TypeConverter::encodeBase64(signer.user_id);
      content_list.emplace_back(user_id_str);
      content_list.emplace_back(signer.pk_cert);
    }
  }

  new_transaction.setContents(content_list);

  new_transaction.refreshSignature();

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