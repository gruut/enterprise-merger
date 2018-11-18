#ifndef GRUUT_HANA_MERGER_APPLICATION_HPP
#define GRUUT_HANA_MERGER_APPLICATION_HPP

#include <boost/asio.hpp>
#include <vector>
#include <queue>

#include "../include/nlohmann/json.hpp"
#include "modules/module.hpp"
#include "modules/transaction_collector/transaction_collector.hpp"
#include "services/signer_pool_manager.hpp"
#include "chain/transaction.hpp"

using namespace std;

namespace gruut {
    enum class MessageType : uint8_t {
        MSG_JOIN = 0x54,
        MSG_CHALLENGE = 0x55,
        MSG_RESPONSE = 0x56,
        MSG_ACCEPT = 0x57,
        MSG_ECHO = 0x58,
        MSG_LEAVE = 0x59,
        MSG_REQ_SSIG = 0xB2,
        MSG_SSIG = 0xB3,
        MSG_BLOCK = 0xB4,
        MSG_ERROR = 0xFF,
    };

    struct Message {
        gruut::MessageType type;
        nlohmann::json data;
    };

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
