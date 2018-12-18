#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_GENERATOR_HPP

#include "../chain/signer.hpp"

namespace gruut {
class TransactionGenerator {
public:
  void generate(vector<Signer> &signers);

private:
  transaction_id_type generateTransactionId();
};
} // namespace gruut
#endif
