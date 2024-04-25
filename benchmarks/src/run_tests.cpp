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
using namespace uprotocol::uuid;
using namespace uprotocol::v1;


auto static inline createProcess(const std::string& name,
                                 const std::string& shm_name,
                                 char** argv,
                                 std::vector<pid_t>& child_pids,
                                 std::vector<shm_data>& shm_vec) -> int {
    argv[0] = new char[name.size() + 1];
    strncpy(argv[0], name.c_str(), name.size());
    argv[0][name.size()] = '\0';
    
    argv[1] = new char[shm_name.size() + 1];
    strncpy(argv[1], shm_name.c_str(), shm_name.size());
    argv[1][shm_name.size()] = '\0';
    
//    std::cout << "name : " << argv[0] << " shm name = " << argv[1] << std::endl;
//        for (size_t i = 0; argv[i] != nullptr; i++) {
//            std::cout << argv[i] << " ";
//        }
//        std::cout << std::endl;
    
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "fork failed" << std::endl;
        return -1;
    } else if (pid == 0) {
        auto child_pid = getpid();
        std::cout << "chaild process : " <<  argv[0] << ":" << argv[1] << " Process id = " << child_pid << " running. " << std::endl;
        if (execvp(argv[0], const_cast<char * const *>(argv)) < 0) {
            std::cerr << "failed to execute command : " << argv[0] << std::endl;
            return -1;
        }
//        sleep(10);
//        std::cout << "chaild process : " << child_pid << " finish. " << std::endl;
    } else {
        child_pids.push_back(pid);
        createSheredMem(name, shm_name, shm_vec);
    }
    
     
    return 0;
}

static inline auto is_valid_name(const std::string &name) -> bool {
    return !name.empty() && std::isdigit(name[0]);
};


auto main(const int argc, char **argv) -> int {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    int loops = NUMBER_OF_LOOPS;
    int message_size = MESSAGE_SIZE;
    int num_of_uri = 10;
    int max_process = MAX_PROCESS;
    auto now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* local_time = std::localtime(&now_time_t);
    std::stringstream s;
    s << std::put_time(local_time, "%y-%m-%d_%H-%M-%S");
    auto path = std::filesystem::current_path() / "benchmarks" / s.str();
    //std::cout << path << std::endl;
    try {
        if (!std::filesystem::create_directory(path)) {
            std::cerr << "directory already exists" << std::endl;
        }
    }catch (const std::filesystem::filesystem_error& e) {
        std::cerr<< "Error creating directory. " << e.what() << std::endl;
    }
    
    auto dir_path = std::filesystem::current_path() / "benchmarks";
    auto dirs = getDirectories(dir_path);
    sort(dirs.begin(), dirs.end());
    
    if (dirs.size() > 8) {
        for (auto i = 0; i < 5; i++) {
            remove_directory(dirs[i]);
        }
    }

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
        num_of_uri = std::strtol(argv[3], &endptr, 10);
        if (NUMBER_OF_MAX_URI < num_of_uri) {
            num_of_uri = NUMBER_OF_MAX_URI;
        }
    }
    auto list_of_servers = createServerPortlist(max_process);
    
    for (auto const& entry : list_of_servers) {
        std::cout << entry.first << " : " << entry.second.first << "  " << entry.second.second << std::endl;
    }
    
    std::cout << "all pub :" << getAllPubKeys(list_of_servers) << std::endl;
    std::cout << "all sub :" << getAllSubKeys(list_of_servers) << std::endl;
    
    auto loops_s = std::to_string(loops);
    if (setenv("NUMBER_OF_MESSAGES", loops_s.c_str(), 1) < 0) {
        std::cout << "failed to set environment variable" << std::endl;
        exit;
    }
    
    auto msg_size_s = std::to_string(message_size);
    if (setenv("MESSAGE_SIZE", msg_size_s.c_str(), 1) < 0) {
        std::cout << "failed to set environment variable" << std::endl;
        exit;
    }
    
    std::vector<std::string> uri_vec {};
    for (auto i = 0; i < num_of_uri; i++) {
        auto u_authority = BuildUAuthority().build();
        auto u_entity = BuildUEntity().setId(i + 1).setMajorVersion(1).build();
        auto u_resource = BuildUResource().setID((i + 1) << 3).build(); //BuildUResource().setID(3).build();
        auto u_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
        auto v8uri = MicroUriSerializer::serialize(u_uri);
        auto s= convertSerializedURItoString(v8uri);
        uri_vec.push_back(s);
    }
    
    char** argv_f = new char*[uri_vec.size() + 3]; // one for name and 1 for nullptr
    for (size_t i = 2; i < uri_vec.size() + 2; i++) {
        argv_f[i] = new char[uri_vec[i - 2].size() + 1];
        strncpy(argv_f[i], uri_vec[i - 2].c_str(), uri_vec[i - 2].size());
        argv_f[i][uri_vec[i - 2].size()] = '\0';
    }
    std::cout << std::endl;
    argv_f[uri_vec.size() + 2] = nullptr;
    std::vector<pid_t> child_pids;
    std::vector<shm_data> shm_vec;
   
    
    std::string name {};
    for (auto i = 0; i < max_process; i++) {
        name = (i % 2 == 0) ?"./benchmarks/pub_test" : "./benchmarks/sub_test";
        std::string s = "app" + std::to_string(i);
        
        argv_f[0] = new char[name.size() + 1]; //application name
        argv_f[1] = new char[s.size() + 1]; //specific app instance
        
        if (createProcess(name, s, argv_f, child_pids, shm_vec) < 0) {
            spdlog::error("Error creating process, {}",  strerror(errno));
            for (auto const & child_pid : child_pids) {
                // terminate all
                kill(child_pid, SIGTERM);
                waitpid(child_pid, nullptr, 0);
            }
            for (auto s : shm_vec) {
                removeSharedMem(s);
            }
            exit(-1);
        }
    }
    
    for (auto const & shm : shm_vec) {
        const char* message = "Start";
        std::memcpy(shm.ptr, message, strlen(message) + 1);
    }
    
    std::string stop = "Stop";
    while (true) {
        auto i = 0;
        for (auto const &shm: shm_vec) {
            if (shm.shm_name.substr(1, 1) == "p") {
                //auto char_str = static_cast<char*>(shm.ptr);
                std::string message = std::string(static_cast<char*>(shm.ptr));
                if (stop == message) {
                    i++;
                }
            }
        }
        if (i == (max_process / 2)) {
            for (auto const &shm: shm_vec) {
                const char * message = "Stop";
                if (shm.shm_name.substr(1, 1) == "s") {
                    std::cout << "send stop to : " << shm.shm_name << std::endl;
                    std::memcpy(shm.ptr, message, std::strlen(message) + 1);
                }
            }
            break;
        }
    }
    
    for (auto child_pid : child_pids) {
        waitpid(child_pid, nullptr, 0);
        std::cout << "chiled process : " << child_pid << " terminated" << std::endl;
    }
    
    // start process statistics
    auto list  =  getFilesFromDir(path);
    std::vector<double> pub_vec {};
    std::vector<double> sub_vec {};
    for (auto const& l :list) {
        std::filesystem::path local_path(l);
        auto file_name = local_path.filename().string();
        if (file_name.substr(0,1) == "p") {
            readFileToVec(local_path, pub_vec);
        } if (file_name.substr(0,1) == "s") {
            readFileToVec(local_path, sub_vec);
        }
    }
    
    auto sub_stat = getStats(sub_vec);
    auto pub_stat = getStats(pub_vec);
    spdlog::info("{}", printHeader());
    spdlog::info("{}", printStat("subscribe", sub_stat.value()));
    spdlog::info("{}", printStat("publish", pub_stat.value()));
    
    for (auto s : shm_vec) {
        removeSharedMem(s);
    }
    
}
