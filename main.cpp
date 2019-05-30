#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "zksnark_server.h"
#include "easylogging++.h"
#include "librustzcash.h"

INITIALIZE_EASYLOGGINGPP

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

void RunServer() {
    std::string server_address("0.0.0.0:60051");
    TronZksnarkServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG(INFO) << "Listening on " << server_address;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

void InitLog() {
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
    defaultConf.setGlobally(el::ConfigurationType::Format, "[%datetime] [%level] [%func](%fbase:%line) %msg");
    defaultConf.setGlobally(el::ConfigurationType::Filename, "/tmp/logs/tron-zksnark.log");
    defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
    defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
    defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "209715200");
    el::Loggers::reconfigureLogger("default", defaultConf);
}

void InitZksnarkParams() {
    LOG(INFO) << "InitZksnarkParams being ...";
    std::string sapling_spend_str("/tmp/params/sapling-spend.params");
    std::string sapling_output_str("/tmp/params/sapling-output.params");
    librustzcash_init_zksnark_params(
        reinterpret_cast<const codeunit*>(sapling_spend_str.c_str()),
        sapling_spend_str.length(),
        "8270785a1a0d0bc77196f000ee6d221c9c9894f55307bd9357c3f0105d31ca63991ab91324160d8f53e2bbd3c2633a6eb8bdf5205d822e7f3f73edac51b2b70c",
        reinterpret_cast<const codeunit*>(sapling_output_str.c_str()),
        sapling_output_str.length(),
        "657e3d38dbb5cb5e7dd2970e8b03d69b4787dd907285b5a7f0790dcc8072f60bf593b32cc2d1c030e00ff5ae64bf84c5c3beb84ddc841d48264b4a171744d028"
    );
    LOG(INFO) << "InitZksnarkParams end";
}

int main() {
    InitLog();
    InitZksnarkParams();
    LOG(INFO) << "Server being ...";
    RunServer();

    return 0;
}