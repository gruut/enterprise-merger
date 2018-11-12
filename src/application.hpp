#ifndef GRUUT_HANA_MERGER_APPLICATION_HPP
#define GRUUT_HANA_MERGER_APPLICATION_HPP
#include <boost/asio.hpp>

namespace gruut {
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
