#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_REQUESTER_HPP

#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/message.hpp"
#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"

#include "../ledger/certificate_ledger.hpp"

#include "../utils/periodic_task.hpp"

#include "block_generator.hpp"
#include "message_proxy.hpp"
#include "signature_requester.hpp"
#include "signer_pool.hpp"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <vector>

namespace gruut {

class SignatureRequester {
public:
  SignatureRequester();
  void requestSignatures();

private:
  void doCreateBlock();
  void sendRequestMessage(std::vector<Signer> &signers);
  std::vector<Signer> selectSigners();
  bool isNewSigner(Signer &signer);

  Transaction genCertificateTransaction(std::vector<Signer> &signers);

  std::unique_ptr<boost::asio::io_service::strand> m_block_gen_strand;
  PeriodicTask m_collect_check_scheduler;
  DelayedTask m_collect_over_scheduler;

  MerkleTree m_merkle_tree;
  BasicBlockInfo m_basic_block_info;

  std::atomic<bool> m_is_collect_running{false};
  size_t m_max_signers;

  CertificateLedger m_cert_ledger;
};
} // namespace gruut
#endif
