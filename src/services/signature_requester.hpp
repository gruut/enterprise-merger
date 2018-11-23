#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <vector>

#include "../application.hpp"
#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/message.hpp"

namespace gruut {
class Transaction;

const int SIGNATURE_COLLECTION_INTERVAL = 10000;
const int SIGNATURE_COLLECT_SIZE = 10;

using Transactions = std::vector<Transaction>;

class SignatureRequester {
public:
  SignatureRequester();

  bool requestSignatures();

private:
  void startSignatureCollectTimer(Transactions &transactions);

  Transactions fetchTransactions();

  PartialBlock makePartialBlock(Transactions &transactions);

  Message makeMessage(PartialBlock &block);

  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  std::thread *m_signature_check_thread;
  bool m_runnable = false;
  MerkleTree m_merkle_tree;
};
} // namespace gruut
#endif
