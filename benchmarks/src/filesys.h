
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

#ifndef UP_ZENOH_EXAMPLE_CPP_FILESYS_H
#define UP_ZENOH_EXAMPLE_CPP_FILESYS_H

#include "utils.h"

#include <filesystem>
#include <fstream>
#include <sstream>


namespace fs = std::filesystem;

static inline auto getCurrentPath() -> std::filesystem::path {
    return fs::current_path();
}

static inline auto getCurrentPathAndAddFileName(std::string &file_name) -> std::filesystem::path {
    return getCurrentPath() / file_name;
}

static inline auto getFilesFromDir(const std::string& directory) -> std::vector<std::string> {
    std::vector<std::string> file_list {};
    
    std::transform(fs::directory_iterator(directory),
                   fs::directory_iterator(),
                   std::back_inserter(file_list),
                   [](std::filesystem::directory_entry entry) {
                       return entry.is_regular_file() ? entry.path().string() : "";
                   });
    
    return file_list;
}
static inline auto getFilesFromDir(std::filesystem::path& directory) -> std::vector<std::string> {
    std::vector<std::string> file_list {};
    
    std::transform(fs::directory_iterator(directory),
                   fs::directory_iterator(),
                   std::back_inserter(file_list),
                   [](std::filesystem::directory_entry entry) {
                       return entry.is_regular_file() ? entry.path().string() : "";
                   });
    
    return file_list;
}

static inline auto getDirectories(std::string& directory) -> std::vector<std::string> {
    std::vector<std::string> file_list {};
    
    std::transform(fs::directory_iterator(directory),
                   fs::directory_iterator(),
                   std::back_inserter(file_list),
                   [](std::filesystem::directory_entry entry) {
                       return entry.is_directory() ? entry.path().string() : "";
                   });
    
    return file_list;
}

static inline auto getDirectories(const std::filesystem::path&  directory) -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> dir_list {};
    
    try {
        if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (std::isdigit((entry.path().filename().string())[0])) {
                    dir_list.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Filesystem error : " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "General error : " << e.what() << std::endl;
    }
   
    return dir_list;
}

static auto inline remove_directory(const std::filesystem::path&  dir) {
    try {
        auto num_of_items_removed = std::filesystem::remove_all(dir);
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Filesystem error : " << e.what() << std::endl;
    }
    return 0;
}

static auto inline getWorkingDir() {
    auto dir_path = std::filesystem::current_path() / "benchmarks";
    auto dirs = getDirectories(dir_path);
    sort(dirs.begin(), dirs.end());
    return dirs[dirs.size() - 1];
}

static auto inline writeVecToFile(const std::filesystem::path& file_name, std::vector<double> &vec) {
//    int flags = O_WRONLY | O_CREAT | O_APPEND;
//    int fd = 0;
//    int fres = 0;
//    
//    do {
//        fd = open(file_name.string().c_str(), flags, 0644);
//        if (fd == -1) {
//            std::cerr << "Can't open file " << file_name.string() << std::endl;
//            return -1;
//        }
//    
//        struct flock fl;
//        fl.l_type = F_WRLCK;
//        fl.l_whence = SEEK_SET;
//        fl.l_start = 0;
//        fl.l_len = 0;
//    
//        fres = fcntl(fd, F_SETLK, &fl);
//        if (fres == -1) {
//            sleep(1);
//        }
//    
//    } while (fres == -1);
    
    std::ofstream output_file(file_name, std::ios::app);
    if (!output_file.is_open()) {
        std::cerr << "Can't open file " << file_name << std::endl;
        return -1;
    }
    
    for (const auto& d : vec) {
//        std::stringstream s {};
//        s <<  std::fixed<< std::setprecision(9) << d << std::endl;
//        write(fd, s.str().c_str(), s.str().size());
//        s.str("");
        output_file << std::fixed<< std::setprecision(9) << d << std::endl;
    }
    output_file.close();
//    close(fd);
    return 0;
}

static auto inline readFileToVec(const std::filesystem::path& file_name, std::vector<double> &vec) {
    std::ifstream input_file(file_name);
    if (!input_file) {
        std::cerr << "Error open file for read : "  << file_name << std::endl;
        return -1;
    }
    
    std::string line;
    while (std::getline(input_file, line)) {
        std::istringstream i(line);
        double val;
        if (i >> val) {
            vec.push_back(val);
        } else {
            std::cerr << "error invalid value (not double) : " << line << std::endl; 
        }
    }
    return 0;
}


#endif //UP_ZENOH_EXAMPLE_CPP_FILESYS_H
