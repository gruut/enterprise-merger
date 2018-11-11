#ifndef GRUUT_HANA_MERGER_COMMUNICATION_HPP
#define GRUUT_HANA_MERGER_COMMUNICATION_HPP

#include <iostream>
#include <memory>
#include "../module.hpp"
#include "../../application.hpp"

namespace gruut {
    class Communication : public Module {
    public:
        virtual void start() {
            start_communication_loop();
        }

    private:
        void start_communication_loop() {
            auto& io_service = Application::app().get_io_service();
            io_service.post([](){

            });
        }
    };
}
#endif //GRUUT_HANA_MERGER_COMMUNICATION_HPP
