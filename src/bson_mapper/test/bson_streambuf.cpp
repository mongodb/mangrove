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

#include <iostream>

using namespace bson_mapper;

/**
 * An object that compares a given BSON document to an original reference.
 * This is used for unit testing.
 */
class doc_validator {
   public:
    doc_validator(bsoncxx::document::value doc) : _doc(doc), _count(0) {
    }

    // Getter for the number of documents compared so far.
    int count() {
        return _count;
    }

    /**
     * This asserts that the given document is the same as the reference document.
     */
    void check(bsoncxx::document::value v) {
        _count++;
        REQUIRE(v.view() == _doc.view());
    }

   private:
    // Keeps track of the number of documents compared by this validator.
    int _count;
    bsoncxx::document::value _doc;
};

TEST_CASE("bson_streambuf can faithfully transfer a document",
          "[bson_mapper::bson_output_streambuf]") {
    // set up test BSON document
    std::string json_str = "{\"a\": 1, \"b\":[1,2,3], \"c\": {\"a\": 1}}";
    auto bson_obj = bsoncxx::from_json(json_str);
    auto bson_view = bson_obj.view();
    const uint8_t *data = bson_view.data();
    int len = bson_view.length();

    // set up document validator
    doc_validator validator = doc_validator(bson_obj);

    // set up stream. The callback passes created documents into the validator object.
    bson_output_streambuf b_buff([&validator](bsoncxx::document::value v) { validator.check(v); });
    std::ostream doc_stream(&b_buff);

    // write document to stream
    doc_stream.write(reinterpret_cast<const char *>(data), len);
    // write document second time
    doc_stream.write(reinterpret_cast<const char *>(data), len);
    REQUIRE(validator.count() == 2);
}
