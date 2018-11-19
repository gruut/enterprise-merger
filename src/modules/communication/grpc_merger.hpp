#pragma once

#include <thread>
#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "protos/protobuf_merger.grpc.pb.h"
#include "protos/protobuf_signer.grpc.pb.h"
#include "../../application.hpp"

namespace gruut {
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

        void runMergerServ(char *port_for_merger);

        void runSignerServ(char *port_for_signer);

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

            void proceed();

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

            void proceed();

        private:
            SignerCommunication::AsyncService *m_pull_service;
            ServerCompletionQueue *m_pull_cq;

            ServerContext m_pull_ctx;

            SignerDataRequest m_pull_request;
            SignerDataReply m_pull_reply;

            ServerAsyncResponseWriter<SignerDataReply> m_pull_responder;

            CallStatus m_pull_status;
        };

        void handleMergerRpcs();
        void handleSignerRpcs();

        std::unique_ptr<ServerCompletionQueue> m_cq_merger;
        MergerCommunication::AsyncService m_service_merger;
        std::unique_ptr<Server> m_server_merger;

        std::unique_ptr<ServerCompletionQueue> m_cq_signer;
        SignerCommunication::AsyncService m_service_signer;
        std::unique_ptr<Server> m_server_signer;
    };

    class MergerRpcClient {
    public:
        void run();

    private:
        bool pushData(std::string &compressed_data, std::unique_ptr<MergerCommunication::Stub> stub);
        bool checkMsgType(MessageType msg_type);
        void sendData(std::string &header_added_data);
    };
}