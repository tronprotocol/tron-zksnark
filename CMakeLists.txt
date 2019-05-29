cmake_minimum_required(VERSION 3.13)
project(tron_zksnark)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

link_directories(lib)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    add_definitions(-D_WIN32_WINNT=0x600)
endif()

add_subdirectory(third_party/grpc EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

get_filename_component(zksnark_proto "third_party/protocol/api/zksnark.proto" ABSOLUTE)
get_filename_component(zksnark_proto_dependencies "third_party/protocol/core/*.proto" ABSOLUTE)
get_filename_component(zksnark_proto_path "${zksnark_proto}" PATH)
get_filename_component(zksnark_proto_path "${zksnark_proto_path}" PATH)

# Generated sources
set(zksnark_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/api/zksnark.pb.cc")
set(zksnark_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/api/zksnark.pb.h")
set(zksnark_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/api/zksnark.grpc.pb.cc")
set(zksnark_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/api/zksnark.grpc.pb.h")
FILE(GLOB zksnark_src_dependencies ${CMAKE_CURRENT_BINARY_DIR}/core/*.cc)

add_custom_command(
        OUTPUT "${zksnark_proto_srcs}" "${zksnark_proto_hdrs}" "${zksnark_grpc_srcs}" "${zksnark_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${zksnark_proto_path}"
        -I "${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${zksnark_proto}"
        "${zksnark_proto_dependencies}"
        DEPENDS "${zksnark_proto}"
)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(tron_zksnark main.cpp
        ${zksnark_proto_srcs}
        ${zksnark_grpc_srcs}
        ${zksnark_src_dependencies}
        )
target_link_libraries(tron_zksnark
        ${_GRPC_GRPCPP_UNSECURE}
        ${_PROTOBUF_LIBPROTOBUF})
