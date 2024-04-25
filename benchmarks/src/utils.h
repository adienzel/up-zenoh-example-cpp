
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

#ifndef UP_ZENOH_EXAMPLE_CPP_UTILS_H
#define UP_ZENOH_EXAMPLE_CPP_UTILS_H

#include <iostream>
#include <tuple>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h> 
#include <vector>
#include <set>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <optional>
#include <random>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <utility>
#include <unordered_map>


#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/uri/builder/BuildUUri.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>

#include <up-core-api/ustatus.pb.h>
#include <up-core-api/uri.pb.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/uri/serializer/MicroUriSerializer.h>



#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

const char* URI_STRING_FORMAT = "/test.app%d/1/dummy";
constexpr int NUMBER_OF_LOOPS = 20;
constexpr int MESSAGE_SIZE = 100;
constexpr int SIZE_OF_NAME = 10;

constexpr int MAX_PROCESS = 2;
constexpr int NUMBER_OF_MAX_URI = 15;

struct shm_data {
    int shm_id;
    std::string shm_name;
    void* ptr;
};

constexpr size_t SIZE = 128;
const std::string PUB = "pub";
const std::string SUB = "sub";

auto static inline createServerPortlist(int num_of_process = MAX_PROCESS) -> std::unordered_map<std::string, std::pair<std::string, std::string>> {
    std::unordered_map<std::string, std::pair<std::string, std::string>> conf_map;
    std::string key_base = "app";
    //std::string value_base =  "[\"tcp/127.0.0.1:76"; //["tcp/192.168.1.1:7447", "tcp/192.168.1.2:7447"], //this is tcp
    std::string value_base =  "[\"unixpipe/app";
    
    for (auto i = 0; i < num_of_process; i++) {
        std::string app_type = (i % 2 == 0) ? PUB : SUB;
        std::string key = key_base + std::to_string(i);
        auto value = std::make_pair(app_type, value_base + std::to_string(i) + ".pipe" + "\"]");
        conf_map[key] = value;
    }
    
    return conf_map;
}

auto static inline getAllTypeKeys(std::unordered_map<std::string, std::pair<std::string, std::string>> conf_map, std::string filter) -> std::string {
    std::stringstream s;
    s <<  "[";
    for (auto const &entry : conf_map) {
        if (entry.second.first == filter) {
            
            std::string addr = entry.second.second;
            if (addr.empty()) continue;
            addr.pop_back();
            addr.erase(addr.begin());
            s << addr << ",";
        }
    }
    
    //remove the last ","
    auto str = s.str();
    if (!str.empty()) {
        str.pop_back();
        str += "]";
    }
    return str;
}

auto static inline getAllSubKeys(const std::unordered_map<std::string, std::pair<std::string, std::string>>& conf_map) -> std::string {
    return getAllTypeKeys(conf_map, SUB);
}

auto static inline getAllPubKeys(const std::unordered_map<std::string, std::pair<std::string, std::string>>& conf_map) -> std::string {
    return getAllTypeKeys(conf_map, PUB);
}

auto static inline createSheredMem(const std::string& name, const std::string &shm_name, std::vector<shm_data>& shm_vec) -> int {
    shm_data shm {};
    shm.shm_name = "/" + name.substr(13) + "." + shm_name;
    std::cout << __func__ << ":" << __LINE__ << " cmap to " << shm.shm_name <<  " from " << shm_name << std::endl;
    shm.shm_id = shm_open(shm.shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (-1 == shm.shm_id) {
        spdlog::error("Failed to open {} shered memory, {}", shm.shm_name, strerror(errno));
        return -1;
    }
    ftruncate(shm.shm_id, SIZE);
    shm.ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm.shm_id, 0);
    if (MAP_FAILED == shm.ptr) {
        spdlog::error("Failed to map {} shered memory, {}", name, strerror(errno));
        return -1;
    }
    shm_vec.push_back(shm);
    return 0;
}

auto static inline removeSharedMem(const shm_data& shm) -> int {
    if (munmap(shm.ptr, SIZE) == -1) {
        spdlog::error("Failed to UNMAP {} shered memory, {}", shm.shm_name, strerror(errno));
        return -1;
    }
    close(shm.shm_id);
    shm_unlink(shm.shm_name.c_str());
    return 0;
}

struct sheared_mem {
    int number_of_topics_to_use;
    int number_of_lost_packets;
    int number_of_out_of_sequance;
    int data_array_size;
    double duration[];
};

struct Stat_s {
    std::optional<double> mean;
    std::optional<double> median;
    std::optional<double> min;
    std::optional<double> max;
    std::optional<double> std;
    std::optional<double> skew;
    std::optional<double> kurtosis;
    std::optional<double> precentile_90;
    std::optional<double> precentile_95;
    std::optional<double> precentile_99;
    std::optional<double> precentile_75;
};

static inline auto getDuration(struct timespec &end, struct timespec &start) -> double {
    return ((double)end.tv_sec + (double)end.tv_nsec * 1.0e-9) -
           ((double)start.tv_sec + (double)start.tv_nsec * 1.0e-9);
}

static inline auto convertSerializedURItoString(const std::vector<u_int8_t> &uri_vec) -> std::string {
    std::stringstream s;
    s << std::hex << std::setfill('0');
    for (auto e : uri_vec) {
        s << std::setw(2) << static_cast<unsigned>(e);
    }
    return s.str();
}

static inline auto convertHexStringToUint8Vec(const std::string &str) -> std::vector<u_int8_t> {
    std::vector<u_int8_t> res;
    for (size_t i = 0; i < str.size(); i += 2) {
        auto hex_digit = str.substr(i, 2);
        std::stringstream s;
        int val;
        s << std::hex << hex_digit;
        s >> val;
        res.push_back(static_cast<u_int8_t >(val));
    }
    return res;
}

auto static inline createVectorofUUri(size_t size) -> std::vector<uprotocol::v1::UUri> {
    std::vector<uprotocol::v1::UUri> uri_vec {};
    for (auto i = 1; i <= size; i++) {
        auto u_authority = uprotocol::uri::BuildUAuthority().build();
        auto u_entity = uprotocol::uri::BuildUEntity().setId(i).setMajorVersion(1).build();
        auto u_resource = uprotocol::uri::BuildUResource().setID((i + 1) << 3).build();
        auto u_uri = uprotocol::uri::BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
        uri_vec.push_back(u_uri);
    }   
    return uri_vec;
}

static inline auto getUristr(int argc,  char **argv) -> std::vector<std::string> {
    std::vector<std::string> uri_str;
    for (size_t i = 2; i < argc; i++) {
        std::string s = argv[i];
        uri_str.push_back(s);
    }
    return uri_str;
}

static inline auto split(std::string &data, std::string& delimiter) -> std::vector<std::string> {
    std::vector<std::string> split_data {};
    std::size_t prev = 0;
    std::size_t pos = 0;
    
    do {
        pos = data.find(delimiter, prev);
        if (pos == std::string::npos) {
            pos = data.size();
        }
        auto p = data.substr(prev, pos - prev);
        if (!p.empty()) {
            split_data.push_back(p);
        }
        prev = pos+ delimiter.size();
    } while (pos < data.size() && prev < data.size());
    
    return split_data;
}
static inline auto getEnvNumberOfMessages() -> int {
    const char* num_messages = std::getenv("NUMBER_OF_MESSAGES");
    if (num_messages == nullptr) {
        std::cout << "failed to read environment variable NUMBER_OF_MESSAGES" << std::endl;
        exit(-1);
    }
    char *endptr;
    return std::strtol(num_messages, &endptr, 10);
}

static inline auto getEnvMessageSize() -> int {
    const char* message_size = std::getenv("MESSAGE_SIZE");
    if (message_size == nullptr) {
        std::cout << "failed to read environment variable MESSAGE_SIZE" << std::endl;
        exit(-1);
    }
    char *endptr;
    return std::strtol(message_size, &endptr, 10);
}


bool terminate = false;
static inline auto signalHandler(int signal) -> void {
    if (signal == SIGINT) {
        std::cout << "Got Ctrl+C. Exiting..." << std::endl;
        terminate = true;
    } else if (signal == SIGABRT) {
        std::cout << "Got abort(). Exiting..." << std::endl;
        terminate = true;
    } else if (signal == SIGTERM) {
        std::cout << "Got external (kill). Exiting..." << std::endl;
        exit(-3);
    }
    //set flag to stop process and close system
}

template<typename T>
static inline auto getTypeOf(T& t) -> std::string {
    return typeid(t).name();
}

template<typename T>
static inline auto getAverage(std::vector<T>& vec) -> std::optional<double> {
    T sum = std::accumulate(vec.begin(), vec.end(), T{});
    return (vec.size() == 0) ? std::nullopt : std::make_optional(static_cast<double>(sum) / vec.size());
}


template<typename T>
static inline auto getMedian(std::vector<T>& vec) -> std::optional<double> {
    std::sort(vec.begin(), vec.end());
    size_t size = vec.size();
    if (size < 2) {
        return std::nullopt;
    }
    if (0 == size % 2) {
        return static_cast<double>(vec[size / 2 - 1] + vec[size / 2]) / 2.0;
    } else {
        return static_cast<double>(vec[size / 2]);
    }
}



template<typename T>
static inline auto getMedianSorted(std::vector<T>& vec) -> std::optional<double> {
    size_t size = vec.size();
    if (size < 2) {
        return std::nullopt;
    }
    if (0 == size % 2) {
        return static_cast<double>(vec[size / 2 - 1] + vec[size / 2]) / 2.0;
    } else {
        return static_cast<double>(vec[size / 2]);
    }
}

template<typename T>
static inline auto getPercentileFromSortedVec(std::vector<T>& vec, int percentile = 50) -> std::optional<double> {
    if (percentile < 0 || percentile > 100) {
        return std::nullopt;
    }
    
    double index = ((double)percentile / 100) * (vec.size() - 1) + 1;
    if (index == (int)index) {
        return vec[(int)index -1];
    }
    else {
        int i = (int)index;
        double delta = index - i;
        return vec[i - 1] + delta * (vec[i] - vec[i - 1]);
    }
}

template<typename T>
static inline auto getPercentile(std::vector<T>& vec, double percentile = 50) -> std::optional<double> {
    std::sort(vec.begin(), vec.end());
    return getPersentileFromSortedVec(vec, percentile);
}

template<typename T>
static inline auto getVariance(std::vector<T>& vec, double mean) -> std::optional<double> {
    if (vec.size() == 0) {
        return std::nullopt;
    }
    double variance = std::accumulate(vec.begin(), vec.end(), 0.0, [=](double acc, const auto & e){
        return acc + std::pow(e - mean, 2);
    });
    
    variance /= vec.size();
    return variance;
}

template<typename T>
static inline auto getVariance(std::vector<T>& vec) -> std::optional<double> {
    if (vec.size == 0) {
        return std::nullopt;
    }
    return getVariance(vec, getAverage(vec));
}

template<typename T>
static inline auto getStd(std::vector<T>& vec, double mean) -> std::optional<double> {
    auto variance = getVariance(vec, mean);
    if (variance) {
        return std::sqrt(variance.value());
    }
    return std::nullopt;
}

template<typename T>
static inline auto getStd(std::vector<T>& vec) -> std::optional<double> {
    auto variance = getVariance(vec);
    if (variance) {
        return std::sqrt(variance.value());
    }
    return std::nullopt;
}

template<typename T>
static inline auto getSkew(std::vector<T>& vec, double mean) -> std::optional<double> {
    auto size = vec.size();
    if (size < 3) {
        return std::nullopt;
    }
    auto std = getStd(vec, mean);
    if (!std.has_value()) {
        return std::nullopt;
    }
    auto median = getMedianSorted(vec);
    if (!median.has_value()) {
        return std::nullopt;
    }
    
    return (3 * ( mean - median.value())) / std.value();   
}

template<typename T>
static inline auto getKurtosis(std::vector<T>& vec, double mean, double std) -> std::optional<double> {
    auto size = vec.size();
    if (size < 3) {
        return std::nullopt;
    }
    
    return (1 / size) * std::accumulate(vec.begin(), vec.end(), 0.0, [=](double acc, const auto & e){
        return acc + std::pow(((e - mean) / std), 4);
    }) - 3;
}

template<typename T>
static inline auto getKurtosis(std::vector<T>& vec) -> std::optional<double> {
    auto size = vec.size();
    if (size < 3) {
        return std::nullopt;
    }
    auto mean = getAverage(vec);
    if (!mean.has_value()) {
        return std::nullopt;
    }
    auto std = getStd(vec, mean);
    if (!std.has_value()) {
        return std::nullopt;
    }
    return(getKurtosis(vec, mean.value(), std.value()));
}


template<typename T>
static inline auto getStats(std::vector<T>& vec) -> std::optional<Stat_s> {
    if (vec.size()  < 2) {
        return std::nullopt;
    }
    std::sort(vec.begin(), vec.end());
    Stat_s stat {};
    stat.mean = getAverage(vec);
    stat.min = getPercentileFromSortedVec(vec, 0.0);
    stat.max = getPercentileFromSortedVec(vec, 100.0);
    stat.median = getPercentileFromSortedVec(vec, 50.0);
    stat.precentile_75 = getPercentileFromSortedVec(vec, 75.0);
    stat.precentile_90 = getPercentileFromSortedVec(vec, 90.0);
    stat.precentile_95 = getPercentileFromSortedVec(vec, 95.0);
    stat.precentile_99 = getPercentileFromSortedVec(vec, 99.0);
    stat.std = getStd(vec, stat.mean.value());
    stat.skew = getSkew(vec, stat.mean.value());
    stat.kurtosis = getKurtosis(vec, stat.mean.value(), stat.std.value());
    return stat;
}

const char * STATS_HEADER = "Mean\t|Min\t\t|Max\t\t|STD\t\t|SKEW\t\t|Median\t\t|Kurtosis\t|90%\t\t|95%\t\t|99%   \t|";

const static inline auto printHeader() -> std::string {
    std::string str(SIZE_OF_NAME, ' ');
    str += "|";
    str += STATS_HEADER;
    return str ;
    
}
static inline auto printStat(std::string name, Stat_s &stat) -> std::string {
    if (name.size() < SIZE_OF_NAME) {
        name.resize(SIZE_OF_NAME, ' ');
    }
    
    std::stringstream s;
    try {
        s << std::fixed << std::setprecision(9)
          << name.substr(0, SIZE_OF_NAME) << "|"
          << stat.mean.value() << "\t|"
          << stat.min.value() << "\t|"
          << stat.max.value() << "\t|"
          << stat.std.value() << "\t|"
          << stat.skew.value() << "\t|"
          << stat.median.value() << "\t|"
          << stat.kurtosis.value() << "\t|"
          << stat.precentile_90.value() << "\t|"
          << stat.precentile_95.value() << "\t|"
          << stat.precentile_99.value() << " |";
    } catch (const std::bad_optional_access &e) {
        std::cerr << "stat error : " << e.what() << std::endl;
    
    }
    return s.str();
}

auto getRandomInRange(int start, int end) -> int {
    std::random_device device;
    std::default_random_engine rnd_gen(device());
    std::uniform_int_distribution<int> distribution(start, end);
    return distribution(rnd_gen);
}

auto fillbufferWithRandom(uint8_t *buffer, int start_location, size_t size) -> void {
    std::random_device random_device;
    std::default_random_engine rnd_gen(random_device());
    std::uniform_int_distribution<uint8_t> distribution(0, 255);
    for (size_t i = start_location; i < size - start_location; i++) {
        buffer[i] = distribution(rnd_gen);
    }
}

auto generateRandomString(size_t size) -> std::string {
    std::random_device random_device;
    std::default_random_engine rnd_gen(random_device());
    std::uniform_int_distribution<char> lower_case('a', 'z');
    std::uniform_int_distribution<char> upper_case('A', 'Z');
    std::uniform_int_distribution<char> numbers('0', '9');
    std::uniform_int_distribution<int> choice_of(0, 2);
    
    std::string rnd_str {};
    for (size_t i = 0; i < size; i++) {
        switch (choice_of(rnd_gen)) {
            case 0:
                rnd_str += lower_case(rnd_gen);
                break;
            case 1:
                rnd_str += upper_case(rnd_gen);
                break;
            case 2:
                rnd_str += numbers(rnd_gen);
                break;
            default:
                rnd_str += upper_case(rnd_gen);
                break;
        }
    }
    return rnd_str;
}

/**
 * get number of threads
 * we subtruct 1 
 * @return 
 */
static auto inline getNumberOfThreads() -> int {
    auto pid = getpid();
    int num_of_threads = 0;
    char command[64];
    snprintf(command, 64, "ps -T -p %d | wc -l", pid);
    auto fp = popen(command, "r");
    if (fp) {
        fscanf(fp, "%d", &num_of_threads);
        fclose(fp);
        return num_of_threads - 1;
    }
    return -1;
}
//static inline auto printStatHeader() -> std::string {
//    return "Mean\tMin\tMax\tSTD\tSKEW\tMedian\t75%\t90%\t95%\t99%";
//}

#endif //UP_ZENOH_EXAMPLE_CPP_UTILS_H
