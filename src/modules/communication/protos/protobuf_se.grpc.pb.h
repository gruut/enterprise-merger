// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: protobuf_se.proto
#ifndef GRPC_protobuf_5fse_2eproto__INCLUDED
#define GRPC_protobuf_5fse_2eproto__INCLUDED

#include "protobuf_se.pb.h"

#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace grpc_se {

class GruutSeService final {
 public:
  static constexpr char const* service_full_name() {
    return "grpc_se.GruutSeService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status seService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc_se::Reply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>> AsyncseService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>>(AsyncseServiceRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>> PrepareAsyncseService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>>(PrepareAsyncseServiceRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>* AsyncseServiceRaw(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::grpc_se::Reply>* PrepareAsyncseServiceRaw(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status seService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc_se::Reply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>> AsyncseService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>>(AsyncseServiceRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>> PrepareAsyncseService(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>>(PrepareAsyncseServiceRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>* AsyncseServiceRaw(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::grpc_se::Reply>* PrepareAsyncseServiceRaw(::grpc::ClientContext* context, const ::grpc_se::Request& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_seService_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status seService(::grpc::ServerContext* context, const ::grpc_se::Request* request, ::grpc_se::Reply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_seService : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_seService() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_seService() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status seService(::grpc::ServerContext* context, const ::grpc_se::Request* request, ::grpc_se::Reply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestseService(::grpc::ServerContext* context, ::grpc_se::Request* request, ::grpc::ServerAsyncResponseWriter< ::grpc_se::Reply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_seService<Service > AsyncService;
  template <class BaseClass>
  class WithGenericMethod_seService : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_seService() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_seService() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status seService(::grpc::ServerContext* context, const ::grpc_se::Request* request, ::grpc_se::Reply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_seService : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_seService() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_seService() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status seService(::grpc::ServerContext* context, const ::grpc_se::Request* request, ::grpc_se::Reply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestseService(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_seService : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_seService() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::grpc_se::Request, ::grpc_se::Reply>(std::bind(&WithStreamedUnaryMethod_seService<BaseClass>::StreamedseService, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_seService() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status seService(::grpc::ServerContext* context, const ::grpc_se::Request* request, ::grpc_se::Reply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedseService(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::grpc_se::Request,::grpc_se::Reply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_seService<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_seService<Service > StreamedService;
};

}  // namespace grpc_se


#endif  // GRPC_protobuf_5fse_2eproto__INCLUDED
