#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_GENERATOR_HPP

#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

#include <climits>
#include <random>

namespace gruut {
class TransactionGenerator {
public:
  void generate(vector<Signer> &signers);

private:
  transaction_id_type generateTransactionId();
};
} // namespace gruut
#endif
