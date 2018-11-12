#pragma once

#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "../module.hpp"
#include "../../application.hpp"
#include "protos/protobuf_merger.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::Channel;

using grpc::ClientContext;

//grpc
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "protos/protobuf_MtoM.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::Channel;

using grpc::ClientContext;

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

    class MessageHandler {
    public:
        void Run()
        {

        }
    private:
        void sendDataToMerger(std::string &compressd_data)
        {

        }
    };
}
