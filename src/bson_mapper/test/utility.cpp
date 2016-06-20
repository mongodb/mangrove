// Copyright 2016 MongoDB Inc.
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

#include "catch.hpp"

#include <bson_mapper/utility.hpp>

#include <iostream>

using namespace bson_mapper;
using namespace bsoncxx;

class Foo {
   public:
    int a, b, c;

    bool operator==(const Foo &rhs) {
        return (a == rhs.a) && (b == rhs.b) && (c == rhs.c);
    }

    // void serialize(Archive& ar) {
    //     ar(CEREAL_NVP(a), CEREAL_NVP(b), CEREAL_NVP(c), CEREAL_NVP(f));
    // }
};

// set up test BSON documents and objects
std::string json_str = "{\"a\": 1, \"b\":4, \"c\": 9}";
auto doc = from_json(json_str);
auto doc_view = doc.view();

std::string json_str_2 = "{\"a\": 1, \"b\":4, \"c\": 900}";
auto doc_2 = from_json(json_str_2);
auto doc_2_view = doc_2.view();

Foo obj{1, 4, 9};

TEST_CASE("Function to_document can faithfully convert objects to BSON documents.",
          "[mongo_odm::to_document]") {
    document::value val = to_document(obj);
    auto v = val.view();

    REQUIRE(v["a"].get_int32() == obj.a);
    REQUIRE(v["b"].get_int32() == obj.b);
    REQUIRE(v["c"].get_int32() == obj.c);
}

TEST_CASE("Function to_obj can faithfully convert documents to objects.", "[mongo_odm::to_obj]") {
    // Test return-by-value
    Foo obj1 = to_obj<Foo>(doc_view);
    // Test fill-by-reference
    Foo obj2;
    to_obj(doc_view, obj2);

    REQUIRE(doc_view["a"].get_int32() == obj1.a);
    REQUIRE(doc_view["b"].get_int32() == obj1.b);
    REQUIRE(doc_view["c"].get_int32() == obj1.c);
    //
    REQUIRE(doc_view["a"].get_int32() == obj2.a);
    REQUIRE(doc_view["b"].get_int32() == obj2.b);
    REQUIRE(doc_view["c"].get_int32() == obj2.c);
}

TEST_CASE("Function to_optional_obj can convert optional documents to optional objects.",
          "[mongo_odm::to_optional_obj]") {
    auto empty_optional = bsoncxx::stdx::optional<document::value>();
    bsoncxx::stdx::optional<Foo> should_be_empty = to_optional_obj<Foo>(empty_optional);

    REQUIRE(!should_be_empty);

    auto should_be_filled = to_optional_obj<Foo>(bsoncxx::stdx::optional<document::value>(doc));
    REQUIRE(should_be_filled);
    if (should_be_filled) {
    }
    REQUIRE(doc_view["a"].get_int32() == should_be_filled->a);
    REQUIRE(doc_view["b"].get_int32() == should_be_filled->b);
    REQUIRE(doc_view["c"].get_int32() == should_be_filled->c);
}
