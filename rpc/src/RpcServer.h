
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

#ifndef UP_ZENOH_EXAMPLE_CPP_RPCSERVER_H
#define UP_ZENOH_EXAMPLE_CPP_RPCSERVER_H

#include <iostream>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/MicroUriSerializer.h>
#include <up-cpp/transport/datamodel/UMessage.h>

#include <spdlog/spdlog.h>

using namespace uprotocol::utransport;
using namespace uprotocol::uuid;
using namespace uprotocol::uri;
using namespace uprotocol::v1;


class RpcServer : public ZenohUTransport  {
public:
    RpcServer(ZenohSessionManagerConfig &config) : ZenohUTransport(config) {}
    
    ~RpcServer() {}
    
    inline auto getSuccess() -> uprotocol::v1::UStatus {
        return uSuccess_;
    }
    
};

#endif //UP_ZENOH_EXAMPLE_CPP_RPCSERVER_H
