add_library(modules
        module.hpp
        message_fetcher/message_fetcher.hpp
        communication/communication.hpp
        )

set(PREFIX "../../lib/")

#GRPC
set(GRPC_INCLUDE_DIR "${PREFIX}/grpc/include/grpcpp")
set(GRPC_LIB_DIR "${PREFIX}/grpc/libs")
set(GRPC_LIBS
        "${GRPC_LIB_DIR}/libgrpc++.so"
        "${GRPC_LIB_DIR}/libgrpc.so"
        "${GRPC_LIB_DIR}/libgrpc++_cronet.so"
        "${GRPC_LIB_DIR}/libgrpc++_error_details.so"
        "${GRPC_LIB_DIR}/libgrpc++_reflection.so"
        "${GRPC_LIB_DIR}/libgrpc++_unsecure.so"
        "${GRPC_LIB_DIR}/libgrpcpp_channelz.so"
        )

#PROTO BUFFER
set(PROTOBUF_INCLUDE_DIR "${PREFIX}/protobuf/include/google")
set(PROTOBUF_LIB_DIR "${PREFIX}/protobuf/libs")
set(PROTOBUF_LIBS
        "${GRPC_LIB_DIR}/libprotobuf.a")

#MtoM
set(PROTO_PREFIX "communication/protos")

add_library(MtoM_proto STATIC "${PROTO_PREFIX}/protobuf_MtoM.pb.cc" "${PROTO_PREFIX}/protobuf_MtoM.pb.h")
target_link_libraries(MtoM_proto PRIVATE ${PROTO_LIBS})

add_library(MtoM_grpc STATIC "${PROTO_PREFIX}/protobuf_MtoM.grpc.pb.cc" "${PROTO_PREFIX}/protobuf_MtoM.grpc.pb.h")
target_link_libraries(MtoM_grpc PRIVATE MtoM_proto ${GRPC_LIBS})

include_directories(${GRPC_INCLUDE_DIR})
target_link_libraries(modules PRIVATE MtoM_grpc)
set_target_properties(modules PROPERTIES LINKER_LANGUAGE CXX)
