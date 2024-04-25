
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

#include <csignal>
#include <iostream>
#include <spdlog/spdlog.h>
#include <unistd.h> // For sleep

#include <spdlog/spdlog.h>
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/transport/UListener.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/transport/datamodel/UMessage.h>

#include <up-core-api/ustatus.pb.h>
#include <up-core-api/uri.pb.h>
#include "uri.h"

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::v1;

const std::string TIME_URI_STRING = "/test.app/1/milliseconds";
const std::string RANDOM_URI_STRING = "/test.app/1/32bit";
const std::string COUNTER_URI_STRING = "/test.app/1/counter";



bool gTerminate = false;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "Ctrl+C received. Exiting..." << std::endl;
        gTerminate = true;
    }
}

class CustomListener : public UListener {

    public:
        /* in this example the same onReceive callback implementation is used to receive
         * the three different messages , each message is differntiated by the URI
         * it is possible to provide a different onReceive callback for each topic */
        UStatus onReceive(UMessage &umsg) override {
            std::cout << "got message" << __func__ << ":" << __LINE__ << std::endl;
            auto payload = umsg.payload();
            auto attributes = umsg.attributes();
            if (attributes.has_source()) {
                auto uri = attributes.source();
                auto uuri = MicroUriSerializer::serialize(uri);
                auto entity = uri.entity();
                auto resource = uri.resource();
                auto autority = uri.authority();
                auto eid = entity.id();
                auto rid = resource.id();
                
                if (eid == time_id) {
                    const uint64_t *timeInMilliseconds = reinterpret_cast<const uint64_t *>(payload.data());
                    spdlog::info("time = {}", *timeInMilliseconds);
                } else if (eid == rand_id) {
                    const uint32_t *random = reinterpret_cast<const uint32_t *>(payload.data());
                    spdlog::info("random = {}", *random);
                } else if (eid == count_id) {
                    const uint8_t *counter = reinterpret_cast<const uint8_t *>(payload.data());
                    spdlog::info("counter = {}", *counter);
                }
            }
            UStatus status;

            status.set_code(UCode::OK);

            return status;
        }
};

class Subscriber : public  ZenohUTransport {
public:
    
    //Subscriber() : Subscriber(ZenohSessionManagerConfig()) {}
    Subscriber(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    //Subscriber(ZenohSessionManagerConfig config) : ZenohUTransport(config) {}
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
   
    ~Subscriber() {
    }
    
};
/* The sample sub applications demonstrates how to consume data using uTransport -
 * There are three topics that are received - random number, current time and a counter */
int main(int argc, char** argv) {

    signal(SIGINT, signalHandler);
    ZenohSessionManagerConfig config{};
    //config.listenKey = "[\"unixpipe/pub.pipe\"]";
    //config.connectKey = "[\"unixpipe/pub.pipe\"]";
    
    auto sub = new Subscriber(config);
    if (UCode::OK != (sub->getSuccess()).code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
        
    std::vector<std::unique_ptr<CustomListener>> listeners;
    listeners.emplace_back(std::make_unique<CustomListener>());
    listeners.emplace_back(std::make_unique<CustomListener>());
    listeners.emplace_back(std::make_unique<CustomListener>());

    const std::vector<std::string> uriStrings = {
        TIME_URI_STRING,
        RANDOM_URI_STRING,
        COUNTER_URI_STRING
    };

    /* create URI objects from URI strings */
    std::vector<UUri> uris;
    auto timeUri = buildMicrouri(time_id, 2);
    auto randomUri = buildMicrouri(rand_id, 1);
    auto counterUri = buildMicrouri(count_id, 4);
    uris.push_back(timeUri);
    uris.push_back(randomUri);
    uris.push_back(counterUri);

    /* register listeners - in this example the same listener is used for three seperate topics */
    for (size_t i = 0; i < uris.size(); ++i) {
        auto status = sub->registerListener(uris[i], *listeners[i]);
        if (UCode::OK != status.code()){
            spdlog::error("registerListener failed for {}", uriStrings[i]);
            return -1;
        }
    }

    while (!gTerminate) {
        sleep(1);
    }

    for (size_t i = 0; i < uris.size(); ++i) {
        auto status = sub->unregisterListener(uris[i], *listeners[i]);
        if (UCode::OK != status.code()){
            spdlog::error("unregisterListener failed for {}", uriStrings[i]);
            return -1;
        }
    }
 
    delete sub;
    return 0;
}
