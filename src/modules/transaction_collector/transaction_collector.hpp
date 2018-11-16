#pragma once

#include "../../application.hpp"
#include "../../chain/transaction.hpp"
#include "../message_fetcher/message_fetcher.hpp"

#include <iostream>
#include <queue>
#include <boost/asio.hpp>
#include <thread>

namespace gruut {
    constexpr int TRANSACTION_COLLECTION_INTERVAL = 2000;
    using namespace std;

    class TransactionCollector : public Module {
        using TransactionPool = queue<Transaction>;
    public:
        TransactionCollector() : m_timer(Application::app().getIoService()) {}

        void start() override {
            m_runnable = isRunnable();
            startTimer();
        }

    private:
        bool isRunnable() {
            // TOOD: 항상 TransactionCollector가 동작하는 것은 아니다. 스케쥴러에 의해 동작이 중단될 수도 있고, 이미 블럭 생성중이면 중단시켜야 한다.
            return true;
        }

        void startTimer() {
            m_timer.expires_from_now(boost::posix_time::milliseconds(TRANSACTION_COLLECTION_INTERVAL));
            m_timer.async_wait([this](const boost::system::error_code &ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    cout << "Timer was cancelled or retriggered." << endl;
                    this->m_runnable = false;
                } else if (ec.value() == 0) {
                    this->m_runnable = false;
                }

                this->m_worker_thread->join();
            });

            m_runnable = true;
            m_worker_thread = new thread([this](){
                while (this->m_runnable) {
                    auto transaction = MessageFetcher::fetch<Transaction>();
                    m_transaction_pool.push(transaction);
                }
            });
        }

        boost::asio::deadline_timer m_timer;
        bool m_runnable = false;
        thread *m_worker_thread;
        TransactionPool m_transaction_pool;
    };
}
