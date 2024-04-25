
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

#ifndef UP_ZENOH_EXAMPLE_CPP_URI_H
#define UP_ZENOH_EXAMPLE_CPP_URI_H
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h> // For sleep

#include <spdlog/spdlog.h>
#include <up-client-zenoh-cpp/transport/zenohUTransport.h>
#include <up-cpp/uuid/factory/Uuidv8Factory.h>
#include <up-cpp/transport/builder/UAttributesBuilder.h>
#include <up-cpp/uri/serializer/LongUriSerializer.h>
#include <up-cpp/uri/serializer/MicroUriSerializer.h>
#include <up-cpp/uri/builder/BuildUUri.h>

#include <up-cpp/transport/datamodel/UMessage.h>

#include <up-core-api/ustatus.pb.h>
#include <up-core-api/uri.pb.h>


using namespace uprotocol::utransport;
using namespace uprotocol::uri;
using namespace uprotocol::uuid;
using namespace uprotocol::v1;

constexpr int time_id = 1;
constexpr int rand_id = 2;
constexpr int count_id = 3;

static inline auto buildMicrouri(int entity_id, int resource_id) -> UUri {
    auto u_authority = BuildUAuthority().build();
    auto u_entity = BuildUEntity().setId(entity_id).setMajorVersion(1).build();
    auto u_resource = BuildUResource().setID(resource_id).build();
    auto u_uri = BuildUUri().setAutority(u_authority).setEntity(u_entity).setResource(u_resource).build();
    return u_uri;
}

#endif //UP_ZENOH_EXAMPLE_CPP_URI_H
