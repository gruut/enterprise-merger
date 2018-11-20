#include <iostream>

#include "transaction_collector.hpp"
#include "../../services/message_fetcher.hpp"
#include "../../chain/transaction.hpp"
#include "../../application.hpp"

using namespace std;

namespace gruut {
    TransactionCollector::TransactionCollector() {
        m_timer.reset(new boost::asio::deadline_timer(Application::app().getIoService()));
        m_signature_requester = make_shared<SignatureRequester>();
    }

    void TransactionCollector::start() {
        m_runnable = isRunnable();
        startTimer();
        startSignatureRequest();
    }

    bool TransactionCollector::isRunnable() {
        // TOOD: 항상 TransactionCollector가 동작하는 것은 아니다. 스케쥴러에 의해 동작이 중단될 수도 있고, 이미 블럭 생성중이면 중단시켜야 한다.
        return true;
    }

    void TransactionCollector::startTimer() {
        m_timer->expires_after(chrono::milliseconds(TRANSACTION_COLLECTION_INTERVAL));
        m_timer->async_wait([this](const boost::system::error_code &ec) {
            if (ec == boost::asio::error::operation_aborted) {
                cout << "startTimer: Timer was cancelled or retriggered." << endl;
                this->m_runnable = false;
            } else if (ec.value() == 0) {
                this->m_runnable = false;
            } else {
                this->m_runnable = false;
                throw;
            }

            this->m_worker_thread->join();
        });

        m_runnable = true;
        m_worker_thread = new thread([this]() {
            while (this->m_runnable) {
                auto transaction = MessageFetcher::fetch<Transaction>();
                auto& transaction_pool = Application::app().getTransactionPool();
                transaction_pool.push_back(transaction);
            }
        });
    }

    void TransactionCollector::startSignatureRequest() {
        bool request_result = m_signature_requester->requestSignatures();
        if (!request_result) throw;
    }
}