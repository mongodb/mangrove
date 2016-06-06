// Copyright 2014 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <bsoncxx/document/value.hpp>

#include <bson_mapper/config/prelude.hpp>

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

class BSON_MAPPER_API file {
   public:
    bsoncxx::document::value encode(const char* str, int x);
};

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
