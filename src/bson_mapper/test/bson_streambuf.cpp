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

#include <bson_mapper/bson_streambuf.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

using namespace bson_mapper;

/**
 * A callable object that represents a callback that inserts a document into a collection.
 * The object is constructed with a given collection, and when called inserts document values into
 * that collection.
 *
 * This is used to test the bson_output_streambuf class.
 */
class insert_callback {
   public:
    insert_callback(mongocxx::collection c) : _c(c) {
    }

    /**
     * Inserts the given document value into the collection.
     */
    void operator()(bsoncxx::document::value v) {
        _c.insert_one(v.view());
    }

   private:
    mongocxx::collection _c;
};

TEST_CASE("bson_streambuf can insert a document into a collection",
          "[bson_mapper::bson_output_streambuf]") {
    // init database
    mongocxx::instance inst{};
    mongocxx::client conn{mongocxx::uri{}};
    auto collection = conn["testdb"]["testcollection"];
    std::function<void(bsoncxx::document::value)> cb = insert_callback(collection);

    // set up test BSON document
    std::string json_str = "{\"a\": 1, \"b\":[1,2,3], \"c\": {\"a\": 1}}";
    auto bson_obj = bsoncxx::from_json(json_str);
    auto orig_view = bson_obj.view();
    const uint8_t *data = orig_view.data();
    int len = orig_view.length();

    // set up stream
    bson_output_streambuf b_buff(cb);
    std::ostream doc_stream(&b_buff);

    // print coll's document count before insert
    auto cursor = collection.find({});
    int original_count = std::distance(cursor.begin(), cursor.end());

    // write document to stream
    doc_stream.write(reinterpret_cast<const char *>(data), len);

    cursor = collection.find({});
    int first_insert_count = std::distance(cursor.begin(), cursor.end());
    REQUIRE(first_insert_count == original_count + 1);

    // write document second time
    doc_stream.write(reinterpret_cast<const char *>(data), len);

    cursor = collection.find({});
    int second_insert_count = std::distance(cursor.begin(), cursor.end());
    REQUIRE(second_insert_count == first_insert_count + 1);
}
