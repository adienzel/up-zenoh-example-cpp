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
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h> // For sleep

#include <spdlog/spdlog.h>
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/transport/datamodel/UMessage.h>

#include <up-core-api/ustatus.pb.h>
#include <up-core-api/uri.pb.h>
#include "uri.h"

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
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

static inline auto getTime() -> std::uint8_t* {
    auto currentTime = std::chrono::system_clock::now();
    auto duration = currentTime.time_since_epoch();
    auto timeMilli = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    static std::uint8_t buf[8];
    std::memcpy(buf, &timeMilli, sizeof(timeMilli));
    return buf;
}

static inline std::uint8_t* getRandom() {
    
    int32_t val = std::rand();
    static std::uint8_t buf[4];
    std::memcpy(buf, &val, sizeof(val));

    return buf;
}

static inline std::uint8_t* getCounter() {
    static std::uint8_t counter = 0;
    ++counter;

    return &counter;
}


class Publisher : public  ZenohUTransport {
public :
    Publisher(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {
    }
    
    UCode sendMessage(UUri &uri,
                      std::uint8_t *buffer,
                      size_t size) {
        
        auto uuid = Uuidv8Factory::create();
        
        UAttributesBuilder builder(uri, uuid, UMessageType::UMESSAGE_TYPE_PUBLISH, UPriority::UPRIORITY_CS2);
        UAttributes attributes = builder.build();
        
        UPayload payload(buffer, size, UPayloadType::VALUE);
        
        UMessage umsg(payload, attributes);
        
        //virtual uprotocol::v1::UStatus send(const uprotocol::utransport::UMessage &message) = 0;
        UStatus status = send(umsg);
        if (UCode::OK != status.code()) {
            spdlog::error("send.send failed");
            return UCode::UNAVAILABLE;
        }
        return UCode::OK;
    }
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
      return uSuccess_;
    }
    
    ~Publisher() {
    }
};
/* The sample pub applications demonstrates how to send data using uTransport -
 * There are three topics that are published - random number, current time and a counter */
int main(int argc, char **argv) {

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
    
    
    Publisher *pub = new Publisher(config); 
    if (UCode::OK != (pub->getSuccess()).code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
    /* Create URI objects from string URI*/
    //auto timeUri = LongUriSerializer::deserialize(TIME_URI_STRING);
    auto timeUri = buildMicrouri(time_id, 2);
    //auto randomUri = LongUriSerializer::deserialize(RANDOM_URI_STRING);
    auto randomUri = buildMicrouri(rand_id, 1);
    //auto counterUri = LongUriSerializer::deserialize(COUNTER_URI_STRING);
    auto counterUri = buildMicrouri(count_id, 4);

    while (!gTerminate) {
        /* send current time in milliseconds */
        if (UCode::OK != pub->sendMessage(timeUri, getTime(), 8)) {
            spdlog::error("sendMessage failed");
            break;
        }

        /* send random number */
        if (UCode::OK != pub->sendMessage(randomUri, getRandom(), 4)) {
            spdlog::error("sendMessage failed");
            break;
        }

        /* send counter */
        if (UCode::OK != pub->sendMessage(counterUri, getCounter(), 1)) {
            spdlog::error("sendMessage failed");
            break;
        }
        
        sleep(1);
    }

     /* Terminate zenoh utransport */
    delete pub;
 
    return 0;
}
