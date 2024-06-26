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
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/MicroUriSerializer.h>
#include <up-cpp/transport/datamodel/UMessage.h>

#include <up-client-zenoh-cpp/client/upZenohClient.h>
#include <up-client-zenoh-cpp/rpc/zenohRpcClient.h>

#include "RpcServer.h"

#include <spdlog/spdlog.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uuid;
using namespace uprotocol::uri;
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

class RpcListener : public UListener {
    public:
    
//    RpcListener(void *context) : context_(context) {}
    
    UStatus onReceive(UMessage &rcv_umsg) override {
        std::cout << __FILE__ << ":" << __func__ << ":" << __LINE__ << " Got Rpc request\n";
        /* Construct response payload with the current time */
        auto currentTime = std::chrono::system_clock::now();
        auto duration = currentTime.time_since_epoch();
        uint64_t currentTimeMilli = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        auto payload = rcv_umsg.payload();
        auto request_attributes = rcv_umsg.attributes();
        
        UPayload responsePayload(reinterpret_cast<const uint8_t*>(&currentTimeMilli), sizeof(currentTimeMilli), UPayloadType::VALUE);
        
        /* Build response attributes - the same UUID should be used to send the response 
         * it is also possible to send the response outside of the callback context */
        //auto uuid = Uuidv8Factory::create();
    
    
        auto response = UAttributesBuilder().response(request_attributes.sink(),
                                                      request_attributes.source(),
                                                      request_attributes.priority(),
                                                      request_attributes.id()).build();
        //UAttributes response_attributes = response.build();
        
        /* Send the response */
        UMessage umsg(payload, response);
       // RpcServer *rpcServer = (RpcServer *)context_;
        //return rpcServer->send(umsg);
        auto ret =  UpZenohClient::instance()->send(umsg);
        return ret;
    
    }
private:
    void *context_;
};


/* The sample RPC server applications demonstrates how to receive RPC requests and send a response back to the client -
 * The response in this example will be the current time */
int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

    signal(SIGINT, signalHandler);
    
    ZenohSessionManagerConfig config{};
    config.listenKey = "[\"unixpipe/pub.pipe\"]";
    //config.connectKey = "[\"unixpipe/pub.pipe\"]";
    //config.listenKey = listen_key;
    config.connectKey = "";
    config.qosEnabled = "false";
    config.lowLatency = "true";
    config.scouting_delay = 0;
    
    RpcServer *transport = new RpcServer(config);
    auto  status = transport->getSuccess();
    
    /* init zenoh utransport */
//    status = transport->init();
    if (UCode::OK != status.code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    RpcListener listner((void *)transport);
    
    
    auto u_authority = BuildUAuthority().build();
    auto u_entity = BuildUEntity().setId(8).setMajorVersion(1).build();
    auto u_resource = BuildUResource().setRpcRequest(7).build(); //BuildUResource().setID(3).build();
    auto rpcUri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    
    
    /* register listener to handle RPC requests */
    status = transport->registerListener(rpcUri, listner);
    if (UCode::OK != status.code()) {
        spdlog::error("registerListener failed");
        return -1;
    }

    while (!gTerminate) {
        sleep(1);
    }

    status = transport->unregisterListener(rpcUri, listner);
    if (UCode::OK != status.code()) {
        spdlog::error("unregisterListener failed");
        return -1;
    }

    /* term zenoh utransport */
    delete transport;
    return 0;
}
