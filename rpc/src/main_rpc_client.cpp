/*
 * Copyright (c) 2024 General Motors GTO LLC
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: 2024 General Motors GTO LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <iostream>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <future>
#include <spdlog/spdlog.h>
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-client-zenoh-cpp/client/upZenohClient.h>
#include <up-client-zenoh-cpp/rpc/zenohRpcClient.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/transport/datamodel/UMessage.h>

#include <up-core-api/ustatus.pb.h>
#include <up-core-api/uri.pb.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
using namespace uprotocol::v1;
using namespace uprotocol::client;
using namespace uprotocol::rpc;

bool gTerminate = false;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "Ctrl+C received. Exiting..." << std::endl;
        gTerminate = true; 
    }
}

class RpCDemoClient : public UpZenohClient {
public:
    RpCDemoClient(ZenohSessionManagerConfig &config) : UpZenohClient(config) {}
//    RpCDemoClient() : ZenohRpcClient() {}
    ~RpCDemoClient() {}
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return rpcSuccess_;
    }
    
    UPayload sendRPC(UUri &uri) {
        //static UAttributesBuilder request(const uprotocol::v1::UUri& source, const uprotocol::v1::UUri& sink, uprotocol::v1::UPriority priority, int32_t ttl) {
        auto u_authority = BuildUAuthority().build();
        auto u_entity = BuildUEntity().setId(9).setMajorVersion(1).build();
        auto u_resource = BuildUResource().setID(6).build(); //BuildUResource().setID(3).build();
        auto sink_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    
        //auto request_attr = UAttributesBuilder::request(uri, sink_uri, UPriority::UPRIORITY_CS5, 10).build();
//        UAttributes attributes = builder.build();
    
       
        constexpr uint8_t BUFFER_SIZE = 1;
        uint8_t buffer[BUFFER_SIZE] = {0};
        
        UPayload payload(buffer, sizeof(buffer), UPayloadType::VALUE);
        /* send the RPC request , a future is returned from invokeMethod */
        CallOptions callOpt {};
        callOpt.set_priority(UPriority::UPRIORITY_CS5);
        callOpt.set_ttl(150);
        // TODO set the token later
        //callOpt.set_token();
        std::future<RpcResponse> result = this->invokeMethod(uri, payload, callOpt);
    
        if (!result.valid()) {
            spdlog::error("Future is invalid");
            return UPayload(nullptr, 0, UPayloadType::UNDEFINED);
        }
        /* wait for the future to be fullfieled - it is possible also to specify a timeout for the future */
        result.wait();
        auto res = result.get();
    
        if (UCode::OK != res.status.code()) {
            return UPayload(nullptr, 0, UPayloadType::UNDEFINED);
        }
    
        return res.message.payload();
    }
};

/* The sample RPC client applications demonstrates how to send RPC requests and wait for the response -
 * The response in this example will be the current time */
int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
   
    signal(SIGINT, signalHandler);
    ZenohSessionManagerConfig config{};
    //config.listenKey = "[\"unixpipe/pub.pipe\"]";
    config.connectKey = "[\"unixpipe/pub.pipe\"]";
    config.listenKey = "";
    //config.connectKey = "[\"unixpipe/pub.pipe\"]";
    //config.listenKey = listen_key;
    //config.connectKey = "";
    config.qosEnabled = "false";
    config.lowLatency = "true";
    config.scouting_delay = 0;
    
    RpCDemoClient *rpc = new RpCDemoClient(config);
    if (UCode::OK != rpc->getSuccess().code()) {
        spdlog::error("init failed");
        return -1;
    }
    
    auto u_authority = BuildUAuthority().build();
    auto u_entity = BuildUEntity().setId(8).setMajorVersion(1).build();
    auto u_resource = BuildUResource().setRpcRequest(7).build(); //BuildUResource().setID(3).build();
    auto rpcUri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    
    //auto rpcUri = LongUriSerializer::deserialize("/test_rpc.app/1/rpc.milliseconds");

    while (!gTerminate) {

        auto response = rpc->sendRPC(rpcUri);

        uint64_t milliseconds = 0;
        if (response.data() == nullptr) {
            spdlog::error("Received error");
        }
        if (response.data() != nullptr && response.size() >= sizeof(uint64_t)) {
            memcpy(&milliseconds, response.data(), sizeof(uint64_t));
            spdlog::info("Received = {}", milliseconds);
        }

        sleep(1);
    }
    
    delete rpc;

    return 0;
}
