#pragma once

#include "../module.hpp"
#include "grpc_merger.hpp"

namespace gruut {

    class Communication : public Module {
    public:
        virtual void start() {
            startCommunicationLoop();
        }

    private:
        void startCommunicationLoop() {
            auto& io_service = Application::app().get_io_service();
            io_service.post([](){

            });
        }
    };
}
