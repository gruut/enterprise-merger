#ifndef GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP

#include <boost/asio.hpp>
#include <vector>
#include <queue>

#include "modules/module.hpp"
#include "services/signer_pool_manager.hpp"
#include "chain/transaction.hpp"
#include "chain/message.hpp"

using namespace std;

namespace gruut {
    using InputQueue = shared_ptr<queue<Message>>;
    using OutputQueue = shared_ptr<queue<Message>>;
    using TransactionPool = vector<Transaction>;

    class Application {
    public:
        static Application &app() {
            static Application application;
            return application;
        }

        Application(Application const &) = delete;

        Application operator=(Application const &) = delete;

        boost::asio::io_service &getIoService();

        InputQueue &getInputQueue();

        OutputQueue &getOutputQueue();

        SignerPoolManager &getSignerPoolManager();

        TransactionPool &getTransactionPool();

        void start(const vector<shared_ptr<Module>> &modules);

        void exec();

        void quit();

    private:
        shared_ptr<boost::asio::io_service> m_io_serv;
        InputQueue m_input_queue;
        OutputQueue m_output_queue;
        shared_ptr<gruut::SignerPoolManager> m_signer_pool_manager;
        shared_ptr<TransactionPool> m_transaction_pool;
        Application();

        ~Application() {}
    };
}
#endif
