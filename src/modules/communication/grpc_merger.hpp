#pragma once

#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "grpc_util.hpp"
#include "../../application.hpp"
#include "../../../include/thread_pool.hpp"
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::Channel;

using grpc::ClientContext;

using grpc_merger::MergerCommunication;
using grpc_merger::MergerDataRequest;
using grpc_merger::MergerDataReply;

using grpc_signer::SignerCommunication;
using grpc_signer::SignerDataRequest;
using grpc_signer::SignerDataReply;

namespace gruut {

    enum class CallStatus {
        CREATE, PROCESS, FINISH
    };

    class MergerRpcServer {
    public:
        ~MergerRpcServer() {
            m_server_merger->Shutdown();
            m_cq_merger->Shutdown();

            m_server_signer->Shutdown();
            m_cq_signer->Shutdown();
        }

        void runMergerServ(char *port_for_merger) {
            std::string port_num(port_for_merger);
            std::string server_address("0.0.0.0:");
            server_address += port_num;

            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

            builder.RegisterService(&m_service_merger);
            m_cq_merger = builder.AddCompletionQueue();
            m_server_merger = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << " for Merger" << std::endl;

            handleMergerRpcs();
        }

        void runSignerServ(char *port_for_signer) {
            std::string port_num(port_for_signer);
            std::string server_address("0.0.0.0:");
            server_address += port_num;

            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

            builder.RegisterService(&m_service_signer);
            m_cq_signer = builder.AddCompletionQueue();
            m_server_signer = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << " for Signer" << std::endl;

            handleSignerRpcs();
        }

    private:
        class CallData {
        public:
            virtual void proceed() = 0;
        };

        class ReceiveData final : public CallData {
        public:
            ReceiveData(MergerCommunication::AsyncService *service, ServerCompletionQueue *cq)
                    : m_service(service), m_cq(cq), m_responder(&m_ctx), m_receive_status(CallStatus::CREATE) {

                proceed();
            }

            void proceed() {
                switch (m_receive_status) {
                    case CallStatus::CREATE: {
                        m_receive_status = CallStatus::PROCESS;
                        m_service->RequestpushData(&m_ctx, &m_request, &m_responder, m_cq,
                                                        m_cq, this);
                    }
                    break;

                    case CallStatus::PROCESS: {
                        new ReceiveData(m_service, m_cq);

                        std::string raw_data = m_request.data();
                        if(!HeaderController::validateMessage(raw_data)) {
                            m_reply.set_checker(false);
                        }
                        else {
                            int json_size = HeaderController::getJsonSize(raw_data);
                            std::string compressed_data = HeaderController::detachHeader(raw_data);
                            std::string decompressed_data;
                            //MAC verification 필요

                            Compressor::decompressData(compressed_data, decompressed_data, json_size);
                            uint8_t message_type = HeaderController::getMessageType(raw_data);

                            Message msg;
                            msg.type = static_cast<MessageType>(message_type);
                            msg.data = nlohmann::json::parse(decompressed_data);

                            if(JsonValidator::validateSchema(msg.data, msg.type)) {
                                auto input_queue  = Application::app().getInputQueue();
                                input_queue->push(msg);
                                m_reply.set_checker(true);
                            }
                            else {
                                m_reply.set_checker(false);
                            }
                        }
                        m_receive_status = CallStatus::FINISH;
                        m_responder.Finish(m_reply, Status::OK, this);
                    }
                    break;

                    default: {
                        GPR_ASSERT(m_receive_status == CallStatus::FINISH);
                        delete this;
                    }
                    break;
                }
            }

        private:
            MergerCommunication::AsyncService *m_service;
            ServerCompletionQueue *m_cq;

            ServerContext m_ctx;

            MergerDataRequest m_request;
            MergerDataReply m_reply;

            ServerAsyncResponseWriter<MergerDataReply> m_responder;

            CallStatus m_receive_status;
        };

        class PullRequest final : public CallData {
        public:
            PullRequest(SignerCommunication::AsyncService *service, ServerCompletionQueue *cq)
                    : m_pull_service(service), m_pull_cq(cq), m_pull_responder(&m_pull_ctx), m_pull_status(CallStatus ::CREATE) {

                proceed();
            }

            void proceed() {
                switch (m_pull_status) {
                    case CallStatus::CREATE: {
                        m_pull_status = CallStatus::PROCESS;
                        m_pull_service->RequestpullData(&m_pull_ctx, &m_pull_request, &m_pull_responder, m_pull_cq,
                                                        m_pull_cq, this);
                    }
                        break;

                    case CallStatus::PROCESS: {
                        new PullRequest(m_pull_service, m_pull_cq);

                        m_pull_reply.set_message("someData");
                        m_pull_status = CallStatus::FINISH;
                        m_pull_responder.Finish(m_pull_reply, Status::OK, this);
                    }
                        break;

                    default: {
                        GPR_ASSERT(m_pull_status == CallStatus::FINISH);
                        delete this;
                    }
                        break;
                }
            }

        private:
            SignerCommunication::AsyncService *m_pull_service;
            ServerCompletionQueue *m_pull_cq;

            ServerContext m_pull_ctx;

            SignerDataRequest m_pull_request;
            SignerDataReply m_pull_reply;

            ServerAsyncResponseWriter<SignerDataReply> m_pull_responder;

            CallStatus m_pull_status;
        };

        void handleMergerRpcs() {
            new ReceiveData(&m_service_merger, m_cq_merger.get());
            void *tag;
            bool ok;
            while (true) {
                GPR_ASSERT(m_cq_merger->Next(&tag, &ok));
                GPR_ASSERT(ok);
                static_cast<CallData *>(tag)->proceed();
            }
        }

        void handleSignerRpcs() {
            new PullRequest(&m_service_signer, m_cq_signer.get());
            void *tag;
            bool ok;
            while (true) {
                GPR_ASSERT(m_cq_signer->Next(&tag, &ok));
                GPR_ASSERT(ok);
                static_cast<CallData *>(tag)->proceed();
            }
        }

        std::unique_ptr<ServerCompletionQueue> m_cq_merger;
        MergerCommunication::AsyncService m_service_merger;
        std::unique_ptr<Server> m_server_merger;

        std::unique_ptr<ServerCompletionQueue> m_cq_signer;
        SignerCommunication::AsyncService m_service_signer;
        std::unique_ptr<Server> m_server_signer;
    };

    class MergerRpcClient {
    public:
        void run() {
            m_pool =unique_ptr<ThreadPool>(new ThreadPool(5));
            auto output_queue = Application::app().getOutputQueue();

            //아래 if문을 포함하는 무한루프 필요
            if(!output_queue->empty()) {
                 Message msg = output_queue->front();
                 if(checkMsgType(msg.type)) {
                     std::string compressed_json;
                     std::string json_dump = msg.data.dump();
                     Compressor::compressData(json_dump,compressed_json);
                     std::string header_added_data = HeaderController::attachHeader(compressed_json, msg.type);

                     //MAC 붙이는 것 필요

                     sendData(header_added_data);
                 }
            }

        }

    private:
        bool pushData(std::string &compressed_data, unique_ptr<MergerCommunication::Stub> stub) {
            MergerDataRequest request;
            request.set_data(compressed_data);

            MergerDataReply reply;
            ClientContext context;
            Status status = stub->pushData(&context, request, &reply);

            if (status.ok())
                return reply.checker();
            else {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                return false;
            }
        }

        bool checkMsgType(MessageType msg_type){
            return (msg_type==MessageType::MSG_ECHO || msg_type==MessageType::MSG_BLOCK);
        }

        void sendData(std::string &header_added_data){
            //현재는 로컬호스트로 받는곳 지정 해놓음 변경 필요.
            unique_ptr<MergerCommunication::Stub> stub =
                    MergerCommunication::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

            m_pool->enqueue([&](){pushData(header_added_data, move(stub));});
        }
        std::unique_ptr<ThreadPool> m_pool;
    };
}