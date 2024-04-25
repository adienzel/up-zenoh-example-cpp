
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

#ifndef UP_ZENOH_EXAMPLE_CPP_PUB_H
#define UP_ZENOH_EXAMPLE_CPP_PUB_H
#include "utils.h"
#include "filesys.h"
#include <spdlog/spdlog.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
using namespace uprotocol::v1;


class Publisher : public  ZenohUTransport {
public :
    Publisher(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    ~Publisher() {}
    
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
    
};

auto pub(const int loops, int msg_size, int num_of_uri) -> int {
    std::vector<UUri> uri_vec {};
    for (auto i = 0; i < num_of_uri; i++) {
        auto u_authority = BuildUAuthority().build();
        auto u_entity = BuildUEntity().setId(i + 1).setMajorVersion(1).build();
        auto u_resource = BuildUResource().setID((i + 1) << 3).build(); //BuildUResource().setID(3).build();
        auto u_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    
        uri_vec.push_back(u_uri);
    }
    
    ZenohSessionManagerConfig config{};
    //config.listenKey = "[\"unixpipe/pub.pipe\"]";
    //config.connectKey = "[\"unixpipe/pub.pipe\"]";
    Publisher *transport = new Publisher(config);
    if (UCode::OK != (transport->getSuccess()).code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
    std::vector<double> pub_vec {};
    for (auto i = 0; i < loops; i++) {
        std::stringstream s;
        for (auto const& uri : uri_vec) {
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
            UStatus status = transport->send(umsg);
            if (UCode::OK != status.code()) {
                spdlog::error("send.send failed");
                return UCode::UNAVAILABLE;
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (i != 0) {
                pub_vec.push_back(getDuration(end, start));
            }
            s.str("");
        }
    }
    delete transport;
    
    auto pub_stat = getStats(pub_vec);
    spdlog::info("{}", printStat("publish", pub_stat.value()));
    
    return 0;
}

#endif //UP_ZENOH_EXAMPLE_CPP_PUB_H
