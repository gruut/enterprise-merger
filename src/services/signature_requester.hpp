#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/message.hpp"
#include "../chain/signer.hpp"

namespace gruut {
class Transaction;

const int SIGNATURE_COLLECTION_INTERVAL = 10000;
const int MAX_SIGNATURE_COLLECT_SIZE = 1;

using RandomSignerIndices = std::set<int>;
using Transactions = std::vector<Transaction>;
using Signers = std::vector<Signer>;

class SignatureRequester {
public:
  SignatureRequester() = default;

  void requestSignatures();

private:
  void startSignatureCollectTimer(Transactions &transactions);

  Transactions fetchTransactions();

  PartialBlock makePartialBlock(Transactions &transactions);

  void requestSignature(PartialBlock &block, Signers &signers);

  RandomSignerIndices generateRandomNumbers(unsigned int size);

  Signers selectSigners();
  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  MerkleTree m_merkle_tree;
};
} // namespace gruut
#endif
