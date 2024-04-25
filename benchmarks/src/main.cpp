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
#include "sub.h"
#include "pub.h"

#include <spdlog/spdlog.h>

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
    
    
    sub(loops, max_uri);
    pub(loops, message_size, max_uri);
}


