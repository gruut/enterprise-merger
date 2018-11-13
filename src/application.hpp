#ifndef GRUUT_HANA_MERGER_APPLICATION_HPP
#define GRUUT_HANA_MERGER_APPLICATION_HPP

#include <boost/asio.hpp>
#include <vector>
#include <queue>

#include "modules/module.hpp"
#include "../include/nlohmann/json.hpp"

namespace gruut {
    using namespace std;
    enum class InputMessageType {
        TRANSACTION,
        BLOCK,
        SIGNER
    };

    struct Message {
        InputMessageType type;
        nlohmann::json data;
    };

    using InputQueue = shared_ptr<queue<Message>>;
    using OutputQueue = shared_ptr<queue<Message>>;

    class Application {
    public:
        ~Application() {}

        static Application &app() {
            static Application app_instance;
            return app_instance;
        }

        Application(Application const &) = delete;

        Application operator=(Application const &) = delete;

        boost::asio::io_service &get_io_service() { return *m_io_serv; }

        InputQueue &get_input_queue() { return m_input_queue; }

        OutputQueue &getOuputQueue() {return m_output_queue;}

        void start(const vector<shared_ptr<Module>> &modules) {
            try {
                for (auto module : modules) {
                    module->start();
                }
            } catch (...) {
                quit();
                throw;
            }
        }

        void exec() {
            m_io_serv->run();
        }

        void quit() {
            m_io_serv->stop();
        }

    private:
        std::shared_ptr<boost::asio::io_service> m_io_serv;
        InputQueue m_input_queue;
        OutputQueue m_output_queue;

        Application() {
            m_io_serv = make_shared<boost::asio::io_service>();
            m_input_queue = make_shared<queue<Message>>();
            m_output_queue = make_shared<queue<Message>>();
        };
    };
}
#endif
