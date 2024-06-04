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

#include <spdlog/spdlog.h>

#include "utils.h"
#include "filesys.h"


using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::v1;
using namespace uprotocol::uuid;

class CustomListener : public UListener {
public:
    UStatus onReceive(UMessage &umsg) override {
        UStatus status;
        char *endptr;
        struct timespec tm{};
        clock_gettime(CLOCK_MONOTONIC, &tm);
        auto payload = umsg.payload();
        if (payload.isEmpty()) {
            //spdlog::error("Payload is empty");
            status.set_code(UCode::INVALID_ARGUMENT);
            return status;
        }
        auto p = reinterpret_cast<const uint8_t *>(payload.data());
        std::string data = std::string((char *)p);
    
        std::string delimiter = "|";
        auto split_data = split(data, delimiter);
        if (split_data.size() == 0) {
            //spdlog::error("error in payload");
            status.set_code(UCode::INVALID_ARGUMENT);
            return status;
        }
        //this->incCounter();
        // get the time from the payload
        struct timespec sent_time{};
        if (split_data[0].empty()) {
            std::cout << "error in payload time stamp empty" << std::endl;
            status.set_code(UCode::INVALID_ARGUMENT);
            return status;
        }
    
    
        std::string point = ".";
        auto time = split(split_data[0], point);
        if (time.size() != 2) {
            std::cout << "error in payload time stamp not found" << std::endl;
            status.set_code(UCode::INVALID_ARGUMENT);
            return status;
        }

        sent_time.tv_sec = std::strtol(time[0].c_str(), &endptr, 10);
        sent_time.tv_nsec = std::strtol(time[1].c_str(), &endptr, 10);
        //calculate duration
    
        auto duration = getDuration(tm, sent_time);
        duration_vec.push_back(duration);
        count_arraived++;
        messeges_size += data.size();
    
        //get counter from payload
        if (split_data[1].empty()) {
            //spdlog::error("error in payload counter not found");
            status.set_code(UCode::INVALID_ARGUMENT);
            return status;
        }
    
        if (counter == 0) { // get the first counter arrived
            counter = std::strtol(split_data[1].c_str(), &endptr, 10);
        }
        messeges_size += data.size();
        // the application instsance
//        std::cout << "subscriber instance " << split_data[2] 
//                  << " recived message number : " << counter 
//                  << " At duration : " << std::fixed << std::setprecision(9) << duration << " in seconds" << std::endl;
        // check if message arrived in order
    
        // sum message size
        
        status.set_code(UCode::OK);
        return status;
    }
    // add counters and duration calculators
//    void incCounter() {
//        count_arraived++;
//    }
    int count_arraived = 0;
//    int prev_counter = 0;
    long messeges_size = 0;
//    int missed_messages = 0;
    std::vector<double> duration_vec {};
    long counter = 0;

private:
    
};


bool operator<([[maybe_unused]] const UUri& lhs, [[maybe_unused]] const UUri& rhs) {
    return true;
    //return MicroUriSerializer::serialize(lhs) < MicroUriSerializer::serialize(rhs);
}


class Subscriber : public  ZenohUTransport {
public :
    Subscriber() : Subscriber(ZenohSessionManagerConfig()) {}
    Subscriber(ZenohSessionManagerConfig config) : ZenohUTransport(config) {}
//    Subscriber(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    
    ~Subscriber() {}
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
    
};


auto main(const int argc, char **argv) -> int {
//    std::signal(SIGINT, signalHandler);
//    std::signal(SIGTERM, signalHandler);
//    std::signal(SIGABRT, signalHandler);
    
    auto dir = getWorkingDir();
    std::string file_name = "sub-" + (std::string)argv[1];
    auto path = dir / file_name;
    //std::cout << path << std::endl;
//    auto p_id = getpid(); 
//    auto num_messages = getEnvNumberOfMessages();
    
    std::vector<std::string> uri_str = getUristr(argc,  argv);
    
    std::vector<std::unique_ptr<CustomListener>> listeners;
    UUri* uris = new UUri[uri_str.size()];
    
   
    for (ulong i = 0; i < uri_str.size(); i++) {
        auto str = uri_str[getRandomInRange(0, uri_str.size() - 1)];
        auto vec = convertHexStringToUint8Vec(str);
        uris[i] = MicroUriSerializer::deserialize(vec);
    }
    
    std::vector<shm_data> shm_vec;
    if (createSheredMem((std::string(argv[0])), std::string(argv[1]), shm_vec) != 0) {
        spdlog::error("Failed to open shered memory, {}", strerror(errno));
        return -1;
    }

    std::string start = "Start";
    while (true) {
        std::string message = std::string(static_cast<char*>(shm_vec[0].ptr));
        if (start == message) {
            std::cout << "process : " << argv[0] << " app : " << argv[1] << " Got Start" << std::endl;
            break;
        }
        usleep(100);
    }

    ZenohSessionManagerConfig config{};
    auto list_of_servers = createServerPortlist(MAX_PROCESS);
    auto listen_key = list_of_servers[std::string(argv[1])].second;
    //auto listen_key = getAllPubKeys(list_of_servers);
    std::cout << "listening on : " << listen_key << "for : " << argv[1] <<  std::endl;
    spdlog::info("start Subscriber {}:{}", argv[0], argv[1]);
    config.listenKey = listen_key;
    config.connectKey = "";
    config.qosEnabled = "false";
    config.lowLatency = "true";
    config.scouting_delay = 0;
    
//            std::string qosEnabled;
//    std::string lowLatency;
//    
    
    auto *transport = new Subscriber(config);
    if (UCode::OK != transport->getSuccess().code()) {
        spdlog::error("ZenohUTransport init failed");
        return -1;
    }
    
//    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;

    std::vector<UUri> subscription;
//    auto number_of_subscribers = getRandomInRange(4, uri_str.size() - 1);
//    for (auto i = 0; i < number_of_subscribers; i++) {
//        auto uri = uris[getRandomInRange(0, uri_str.size() - 1)];
//        subscription.push_back(uri);
//        listeners.emplace_back(std::make_unique<CustomListener>());
//    }
    
    for (ulong i = 0; i < uri_str.size(); i++) {
        subscription.push_back(uris[i]);
        listeners.emplace_back(std::make_unique<CustomListener>());
    }
         
        //CustomListener listener {};
    auto listener = std::make_unique<CustomListener>();
    for (size_t i = 0; i < subscription.size(); i++) {
        //auto entry = getRandomInRange(0, uri_str.size() - 1);
        auto status = transport->registerListener(subscription[i], *listeners[i]);
//        std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
        if (UCode::OK != status.code()){
//            auto v8uri = MicroUriSerializer::serialize(subscription[i]);
//            auto str = convertSerializedURItoString(v8uri);
//            spdlog::error("registerListener failed for {}", str);
            std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
            delete transport;
            return -1;
        }
    }
    
    //std::cout << "After register app " << argv[0] << ":" << argv[1] << std::endl;
    
    std::string stop = "Stop";
    int counter = 0;
    // start working and wait for signal to stop
    while (true) {

        std::string message = std::string(static_cast<char*>(shm_vec[0].ptr));
        if (stop == message) {
            std::cout << "process : " << argv[0] << " app : " << argv[1] << " Got stop" << std::endl;
            break;
        }
        if (++counter % 10000 == 0) {
            std::cout << "waiting : " << argv[0] << ":" << argv[1] << std::endl;
        }
        
        usleep(100);
    }
    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << " exit start stat" << std::endl;
    
    for (size_t i = 0; i < subscription.size(); i++) {
        std::cout << argv[0] << ":" << argv[1] << " " << (*listeners[i]).count_arraived << " total messages = " << (*listeners[i]).messeges_size << "\n";
        if ((*listeners[i]).duration_vec.size() > 0) {
            writeVecToFile(path, (*listeners[i]).duration_vec);
        }
    }
    
    for (size_t i = 0; i < subscription.size(); i++) {
        auto status = transport->unregisterListener(subscription[i], *listeners[i]);
        if (UCode::OK != status.code()){
            auto v8uri = MicroUriSerializer::serialize(subscription[i]);
            std::string s(v8uri.begin(), v8uri.end());
    
            //spdlog::error("registerListener failed for {}", s);
            return -1;
        }
    }
    
    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
    delete transport;
    std::cout << __func__ << ":" <<  __LINE__ << ":  " << argv[0] << ":" << argv[1] << std::endl;
    
    return 0;
}

