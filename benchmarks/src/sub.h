
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

#ifndef UP_ZENOH_EXAMPLE_CPP_SUB_H
#define UP_ZENOH_EXAMPLE_CPP_SUB_H

#include "utils.h"
#include "filesys.h"
#include <spdlog/spdlog.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::v1;
using namespace uprotocol::uuid;



class CustomListener : public UListener {
public:
    UStatus onReceive(UMessage &umsg) override {
        std::cout << "got message" << __func__ << ":" << __LINE__ << std::endl;
        UStatus status;
        status.set_code(UCode::OK);
        return status;
    }
};


struct SubListener {
    CustomListener listener;
    UUri           uri;
};
class Subscriber : public  ZenohUTransport {
public:
    Subscriber(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
    ~Subscriber() {
    }
    
};

auto sub(const int loops, int num_of_uri) -> int {
    std::vector<SubListener> subscription {};
    CustomListener listener {};
    for (auto i = 0; i < num_of_uri; i++) {
        auto u_authority = BuildUAuthority().build();
        auto u_entity = BuildUEntity().setId(i +1).setMajorVersion(1).build();
        auto u_resource = BuildUResource().setID((i + 1) << 3).build(); //BuildUResource().setID(3).build();
        auto u_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
        //auto v8uri = MicroUriSerializer::serialize(u_uri);
        //std::string s(v8uri.begin(), v8uri.end());
    
        subscription.push_back({listener, u_uri});
    }
    
    ZenohSessionManagerConfig config{};
    
    auto *transport = new Subscriber(config);
    if (UCode::OK != (transport->getSuccess()).code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
    std::vector<double> subscribe {};
    std::vector<double> unsubscribe {};
    
    for (auto i = 0; i < loops; i++) {
        struct timespec start{};
        struct timespec end{};
        for (auto e : subscription) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            auto status = transport->registerListener(e.uri, e.listener);
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (UCode::OK != status.code()){
    
                auto v8uri = MicroUriSerializer::serialize(e.uri);
                std::string s(v8uri.begin(), v8uri.end());
    
                spdlog::error("registerListener failed for {}", s);
                return -1;
            }
            subscribe.push_back(getDuration(end, start));
        }
    
        for (auto e : subscription) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            auto status = transport->unregisterListener(e.uri, e.listener);
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (UCode::OK != status.code()){
                auto v8uri = MicroUriSerializer::serialize(e.uri);
                std::string s(v8uri.begin(), v8uri.end());
    
                spdlog::error("registerListener failed for {}", s);
                return -1;
            }
            unsubscribe.push_back(getDuration(end, start));
        }
    }
    
    delete transport;

    auto start_sub_stat = getStats(subscribe);
    auto close_sub_stat = getStats(unsubscribe);
    spdlog::info("{}", printHeader());
    spdlog::info("{}", printStat("subscribe", start_sub_stat.value()));
    spdlog::info("{}", printStat("unsubscribe", close_sub_stat.value()));
    
    return 0;
}


#endif //UP_ZENOH_EXAMPLE_CPP_SUB_H
