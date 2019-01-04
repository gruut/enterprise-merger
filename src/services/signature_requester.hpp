#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP

#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/message.hpp"
#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"

#include "block_generator.hpp"
#include "message_proxy.hpp"
#include "signature_requester.hpp"
#include "signer_pool.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <vector>

namespace gruut {

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

  void requestSignature(Signers &signers);

  Signers selectSigners();

  std::unique_ptr<boost::asio::deadline_timer> m_collect_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_check_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_block_gen_strand;

  MerkleTree m_merkle_tree;
  PartialBlock m_partial_block;

  bool m_is_collect_timer_running{false};
  size_t m_max_signers;
};
} // namespace gruut
#endif
