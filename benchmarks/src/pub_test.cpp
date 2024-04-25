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
#include "filesys.h"


using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
using namespace uprotocol::v1;

struct my_uuri {
    UUri uri;
};

// need to support std::set
bool operator<(const my_uuri& lhs, const my_uuri& rhs) {
    return MicroUriSerializer::serialize(lhs.uri) < MicroUriSerializer::serialize(rhs.uri);
}

class Publisher : public  ZenohUTransport {
public :
//    Publisher() : Publisher(ZenohSessionManagerConfig()) {}
//    Publisher(ZenohSessionManagerConfig config) : ZenohUTransport(config) {}
    Publisher(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    ~Publisher() {}


    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
        
    };

auto main(const int argc, char **argv) -> int {
//    std::signal(SIGINT, signalHandler);
//    std::signal(SIGTERM, signalHandler);
//    std::signal(SIGABRT, signalHandler);
    auto num_messages = getEnvNumberOfMessages();
    auto p_id = getpid();
    std::vector<std::string> uri_str = getUristr(argc, argv);
    auto dir = getWorkingDir();
    int msg_size = 800;
    // select number of publishers out of the list will be elected for this run minimum is 2
    //auto number_of_publishers = getRandomInRange(4, uri_str.size() - 1);
    auto number_of_publishers = uri_str.size();
    std::string file_name = "pub-" + (std::string)argv[1];
    auto path = dir / file_name;
    //std::cout << path << std::endl;
    
    std::vector<shm_data> shm_vec;
    if (createSheredMem((std::string(argv[0])), std::string(argv[1]), shm_vec) != 0) {
        spdlog::error("Failed to open shered memory, {}", strerror(errno));
        return -1;
    }
    auto list_of_servers = createServerPortlist(MAX_PROCESS);
    auto connect_key = getAllSubKeys(list_of_servers);
    
    ZenohSessionManagerConfig config{};
    
    config.listenKey = "";
    config.connectKey = connect_key;
    config.qosEnabled = "false";
    config.lowLatency = "true";
    config.scouting_delay = 0;
    
    std::cout << "connect key for publisher " << argv[1] << " is :" << config.connectKey << std::endl;
    
    spdlog::info("start publisher");
    Publisher *pub = new Publisher(config);
    if (UCode::OK != (pub->getSuccess()).code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
    std::set<my_uuri> uri; // using of set to avoid duplicates
    for (auto i = 0; i < number_of_publishers; i++) {
        my_uuri my_uri;
        //auto str = uri_str[getRandomInRange(0, uri_str.size() - 1)];
        auto str = uri_str[i];
        auto vec = convertHexStringToUint8Vec(str);
    
        my_uri.uri = MicroUriSerializer::deserialize(vec);
 
        auto res = uri.insert(my_uri);
//        if (!res.second) {
//            str = uri_str[getRandomInRange(0, uri_str.size() - 1)];
//            vec.clear();
//            vec = convertHexStringToUint8Vec(str);
//            my_uri.uri = MicroUriSerializer::deserialize(vec);
//            uri.insert(my_uri);
//        }
    }
    
    //open session
    
    //start publishing
    std::vector<double> pub_vec {};
    for (auto i = 0; i < 100000; i++) {
        std::stringstream s;
        for (auto const& u : uri) {
            struct timespec tm{};
            struct timespec start{};
            struct timespec end{};
            clock_gettime(CLOCK_MONOTONIC, &tm);
            s << tm.tv_sec << "." << tm.tv_nsec << "|" << i << "|";
            auto len = s.str().size();
            if (msg_size - len > 0) {
                s << generateRandomString(msg_size - len);
            }
            auto uuid = Uuidv8Factory::create();
             
            UAttributesBuilder builder(u.uri, uuid, UMessageType::UMESSAGE_TYPE_PUBLISH, UPriority::UPRIORITY_CS2);
            UAttributes attributes = builder.build();
    
            UPayload payload((const uint8_t *)(s.str().c_str()), msg_size, UPayloadType::VALUE);
    
            UMessage umsg(payload, attributes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            UStatus status = pub->send(umsg);
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
        usleep(10);
        //sleep here for different pub rate
    }
    
  
    // write results to file
    if (pub_vec.size() > 0) {
        writeVecToFile(path, pub_vec);
    }
    
    
    //close session
    
    const char* message = "Stop";
    std::memcpy(shm_vec[shm_vec.size() - 1].ptr, message, strlen(message) + 1);
    usleep(1000);
    
    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
    delete pub;
    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
    return 0;
}
