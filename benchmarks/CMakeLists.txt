# Copyright (c) 2024 General Motors GTO LLC
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
# SPDX-FileType: SOURCE
# SPDX-FileCopyrightText: 2024 General Motors GTO LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(benchmark VERSION 0.1.0 LANGUAGES CXX)

find_package(up-client-zenoh-cpp REQUIRED)
if(BUILD_UNBUNDLED)
    find_package(zenohc REQUIRED)
    set(ZENOH_LIBRARY zenohc::lib)
else()
    if(CROSS_COMPILE)
        find_package(zenohc REQUIRED)
        set(ZENOH_LIBRARY zenohc::lib)
    else()
        find_library(ZENOH_LIB zenohc)
        set(ZENOH_LIBRARY ${ZENOH_LIB})
    endif()
endif()



# bench
add_executable(benc 
        src/main.cpp
        src/sub.h
        src/pub.h
        src/utils.h
        src/filesys.h)
target_link_libraries(benc
        PRIVATE
        up-client-zenoh-cpp::up-client-zenoh-cpp
        ${ZENOH_LIBRARY}
        )

add_executable(start_time
        src/start_time.cpp
        src/utils.h
        src/filesys.h)
target_link_libraries(start_time
        PRIVATE
        up-client-zenoh-cpp::up-client-zenoh-cpp
        ${ZENOH_LIBRARY}
        )
set_target_properties(start_time PROPERTIES LINKER_LANGUAGE CXX)


add_executable(pub_test
        src/pub_test.cpp
        src/utils.h
        src/filesys.h)
target_link_libraries(pub_test
        PRIVATE
        up-client-zenoh-cpp::up-client-zenoh-cpp
        ${ZENOH_LIBRARY}
        )
set_target_properties(pub_test PROPERTIES LINKER_LANGUAGE CXX)

add_executable(sub_test
        src/sub_tests.cpp
        src/utils.h
        src/filesys.h)
target_link_libraries(sub_test
        PRIVATE
        up-client-zenoh-cpp::up-client-zenoh-cpp
        ${ZENOH_LIBRARY}
        )
set_target_properties(sub_test PROPERTIES LINKER_LANGUAGE CXX)

add_executable(run_tests
        src/run_tests.cpp
        src/utils.h
        src/filesys.h)
target_link_libraries(run_tests
        PRIVATE
        up-client-zenoh-cpp::up-client-zenoh-cpp
        ${ZENOH_LIBRARY}
        )
set_target_properties(run_tests PROPERTIES LINKER_LANGUAGE CXX)

