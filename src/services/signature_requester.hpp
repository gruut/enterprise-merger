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

#include "../config/config.hpp"

namespace gruut {
class Transaction;

using RandomSignerIndices = std::set<int>;
using Transactions = std::vector<Transaction>;
using Signers = std::vector<Signer>;

class SignatureRequester {
public:
  SignatureRequester();

  void requestSignatures();

  void checkProcess();

private:
  void startSignatureCollectTimer();

  void stopCollectTimerAndCreateBlock();

  Transactions fetchTransactions();

  PartialBlock makePartialBlock(Transactions &transactions);

  void requestSignature(PartialBlock &block, Signers &signers);

  Signers selectSigners();

  std::unique_ptr<boost::asio::deadline_timer> m_collect_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_check_timer;
  MerkleTree m_merkle_tree;

  bool m_is_collect_timer_running{false};
  size_t m_max_signers;
};
} // namespace gruut
#endif
