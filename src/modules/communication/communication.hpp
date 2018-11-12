#pragma once

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

    class ServerForMerger{
    };

    class MergerRpcClient {
    public:
        MergerRpcClient(){}

        bool sendData(std::string &compressd_data){
            return false;
        }

    private:
    };
}
