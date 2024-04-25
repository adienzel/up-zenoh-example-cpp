// /*
//  * Copyright (c) 2024 General Motors GTO LLC
//  *
//  * Licensed to the Apache Software Foundation (ASF) under one
//  * or more contributor license agreements.  See the NOTICE file
//  * distributed with this work for additional information
//  * regarding copyright ownership.  The ASF licenses this file
//  * to you under the Apache License, Version 2.0 (the
//  * "License"); you may not use this file except in compliance
//  * with the License.  You may obtain a copy of the License at
//  *
//  *   http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing,
//  * software distributed under the License is distributed on an
//  * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  * KIND, either express or implied.  See the License for the
//  * specific language governing permissions and limitations
//  * under the License.
//  * SPDX-FileType: SOURCE
//  * SPDX-FileCopyrightText: 2024 General Motors GTO LLC
//  * SPDX-License-Identifier: Apache-2.0
//  *
//

#include "utils.h"
#include <spdlog/spdlog.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
using namespace uprotocol::v1;

class Session : public  ZenohUTransport {
public :
    
    Session(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    ~Session() {}
    
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
    
};



auto start_session(const int loops, int msg_size, int max_uri) -> void {
    std::vector<double> open_session {};
    std::vector<double> close_session {};
    std::vector<double> pub_vec {};
    
    for (auto i = 0; i < loops; i++) {
        struct timespec start{};
        struct timespec end{};
    
        ZenohSessionManagerConfig config{};
        auto list_of_servers = createServerPortlist(MAX_PROCESS);
        auto connect_key = getAllSubKeys(list_of_servers);
        config.listenKey = "[\"unixpipe/test.pipe\"]";
        config.connectKey = "";
        config.qosEnabled = "false";
        config.lowLatency = "true";
        config.scouting_delay = 0;
    
    
        clock_gettime(CLOCK_MONOTONIC, &start);
        auto *session = new Session(config);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (UCode::OK != session->getSuccess().code()) {
            spdlog::error("ZenohUTransport init failed");
            return;
        }
     
        open_session.push_back(getDuration(end, start));
    
        std::vector<UUri> uri_vec {};
        for (auto i = 0; i < max_uri; i++) {
            auto u_authority = BuildUAuthority().build();
            auto u_entity = BuildUEntity().setId(i + 20).setMajorVersion(1).build();
            auto u_resource = BuildUResource().setID((i + 20) << 3).build(); 
            auto u_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    
            uri_vec.push_back(u_uri);
        }
        for (auto const& uri : uri_vec) {
            std::stringstream s;
        
            struct timespec tm{};
            struct timespec start{};
            struct timespec end{};
            clock_gettime(CLOCK_MONOTONIC, &tm);
            s << std::fixed << std::setprecision(9) << tm.tv_sec << "." << tm.tv_nsec << "|" << i << "|";
            auto len = s.str().size();
            if (msg_size - len > 0) {
                s << generateRandomString(msg_size - len);
            }
            auto uuid = Uuidv8Factory::create();
    
            UAttributesBuilder builder(uri, uuid, UMessageType::UMESSAGE_TYPE_PUBLISH, UPriority::UPRIORITY_CS2);
            UAttributes attributes = builder.build();
    
            UPayload payload((const uint8_t *)(s.str().c_str()), msg_size, UPayloadType::VALUE);
    
            UMessage umsg(payload, attributes);
    
        
            clock_gettime(CLOCK_MONOTONIC, &start);
            auto status = session->send(umsg);
            if (UCode::OK != status.code()) {
                spdlog::error("send.send failed");
                return;
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            pub_vec.push_back(getDuration(end, start));
            s.str("");
        }
    
        clock_gettime(CLOCK_MONOTONIC, &start);
        delete session;
        clock_gettime(CLOCK_MONOTONIC, &end);
        close_session.push_back(getDuration(end, start));
    }
    
    auto open_session_stat = getStats(open_session);
    auto close_session_stat = getStats(close_session);
    auto pub_stat = getStats(pub_vec);
    spdlog::info("{}", printHeader());
    spdlog::info("{}", printStat("open session", open_session_stat.value()));
    spdlog::info("{}", printStat("close session", close_session_stat.value()));
    spdlog::info("{}", printStat("first publish", pub_stat.value()));
    
}

auto main(const int argc, char **argv) -> int {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    
    int loops = NUMBER_OF_LOOPS;
    int message_size = MESSAGE_SIZE;
    int max_uri = NUMBER_OF_MAX_URI;
    
    if (argc >= 2) {
        char *endptr;
        loops = std::strtol(argv[1], &endptr, 10);
    }
    if (argc >= 3) {
        char *endptr;
        message_size = std::strtol(argv[2], &endptr, 10);
    }
    if (argc >= 4) {
        char *endptr;
        max_uri = std::strtol(argv[3], &endptr, 10);
    }
    
    start_session(loops, message_size, max_uri);
    return 0;
}

