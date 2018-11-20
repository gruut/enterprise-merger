#include "application.hpp"
#include "modules/module.hpp"
#include "services/signer_pool_manager.hpp"
#include "chain/transaction.hpp"

namespace gruut {
    boost::asio::io_service &Application::getIoService() {
        return *m_io_serv;
    }

    InputQueue &Application::getInputQueue() {
        return m_input_queue;
    }

    OutputQueue &Application::getOutputQueue() {
        return m_output_queue;
    }

    SignerPoolManager &Application::getSignerPoolManager() {
        return *m_signer_pool_manager;
    }

    TransactionPool &Application::getTransactionPool() {
        return *m_transaction_pool;
    }

    SignaturePool &Application::getSignaturePool() {
        return *m_signature_pool;
    }

    void Application::start(const vector<shared_ptr<Module>> &modules) {
        try {
            for (auto module : modules) {
                module->start();
            }
        } catch (...) {
            quit();
            throw;
        }
    }

    void Application::exec() {
        m_io_serv->run();
    }

    void Application::quit() {
        m_io_serv->stop();
    }

    Application::Application() {
        m_io_serv = make_shared<boost::asio::io_service>();
        m_input_queue = make_shared<queue<Message>>();
        m_output_queue = make_shared<queue<Message>>();
        m_signer_pool_manager = make_shared<SignerPoolManager>();
        m_transaction_pool = make_shared<TransactionPool>();
    }
}
