#ifndef GRUUT_HANA_MERGER_APPLICATION_HPP
#define GRUUT_HANA_MERGER_APPLICATION_HPP
#include <boost/asio.hpp>
#include <vector>

#include "module.hpp"

namespace gruut {
    using namespace std;

    class Application {
    public:
        ~Application() {}

        static Application& app() {
            static Application app_instance;
            return app_instance;
        }
        Application(Application const &) = delete;
        Application operator=(Application const &) = delete;

        boost::asio::io_service& get_io_service() { return *m_io_serv; }

        void start(const vector<Module*>& modules) {
            try {
                for(auto module : modules) {
                    module->start();
                }
            } catch(...) {
                quit();
                throw;
            }
        }

        void exec() {

        }

        void quit() {
            m_io_serv->stop();
        }
    private:
        std::shared_ptr<boost::asio::io_service>  m_io_serv;
        Application() {
            m_io_serv = std::make_shared<boost::asio::io_service>();
        };
    };
}
#endif
