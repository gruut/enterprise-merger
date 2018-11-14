#ifndef GRUUT_HANA_MERGER_APPLICATION_HPP
#define GRUUT_HANA_MERGER_APPLICATION_HPP

#include <boost/asio.hpp>
#include <vector>
#include <queue>

#include "modules/module.hpp"
#include "../include/nlohmann/json.hpp"

namespace gruut {

    using namespace std;

    enum class OuputMessageType : uint8_t{
        MSG_CHALLENGE = 0x55,
        MSG_ACCEPT= 0x57,
        MSG_ECHO = 0x58,
        MSG_LEAVE = 0x59,
        MSG_REQ_SSIG = 0xB2,
        MSG_BLOCK = 0xB4,
        MSG_ERROR = 0xFF
    };

    enum class InputMessageType :uint8_t{
        MSG_JOIN = 0x54,
        MSG_RESPONSE = 0x56,
        MSG_ECHO = 0x58,
        MSG_LEAVE = 0x59,
        MSG_SSIG = 0xB3,
        MSG_BLOCK = 0xB4
    };

    struct OutputMessage{
        gruut::OuputMessageType type;
        nlohmann::json data;
    };

    struct InputMessage {
        gruut::InputMessageType type;
        nlohmann::json data;
    };

    using InputQueue = shared_ptr<queue<InputMessage>>;
    using OutputQueue = shared_ptr<queue<OutputMessage>>;

    class Application {
    public:
        ~Application() {}

        static Application &app() {
            static Application app_instance;
            return app_instance;
        }

        Application(Application const &) = delete;

        Application operator=(Application const &) = delete;

        boost::asio::io_service &getIoService() { return *m_io_serv; }

        InputQueue &getInputQueue() { return m_input_queue; }

        OutputQueue &getOutputQueue() {return m_output_queue;}

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
            m_input_queue = make_shared<queue<InputMessage>>();
            m_output_queue = make_shared<queue<OutputMessage>>();
        };
    };
}
#endif
