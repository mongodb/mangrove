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

#include <iostream>

#include <bsoncxx/builder/stream/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/stdx.hpp>

#include <bson_mapper/bson_streambuf.hpp>

#include <mongo_odm/odm_collection.hpp>

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

TEST_CASE("ODM Collection tests, need to format.", "[mongo_odm::odm_collection]") {
    mongocxx::instance::current();
    mongocxx::client conn{mongocxx::uri{}};
    mongocxx::collection coll = conn["testdb"]["testcollection"];
    odm_collection<Foo> foo_coll(coll);

    // set up test BSON documents and objects
    std::string json_str = "{\"a\": 1, \"b\":4, \"c\": 9}";
    auto doc = bsoncxx::from_json(json_str);
    auto doc_view = doc.view();

    std::string json_str_2 = "{\"a\": 1, \"b\":4, \"c\": 900}";
    auto doc_2 = bsoncxx::from_json(json_str_2);
    auto doc_2_view = doc_2.view();

    Foo obj{1, 4, 9};

    // Test to_document
    {
        bsoncxx::document::value val = to_document(obj);
        auto v = val.view();

        REQUIRE(v["a"].get_int32() == obj.a);
        REQUIRE(v["b"].get_int32() == obj.b);
        REQUIRE(v["c"].get_int32() == obj.c);
    }

    // Test to_obj
    {
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

    // Test to_optional_obj
    {
        auto empty_optional = mongocxx::stdx::optional<bsoncxx::document::value>();
        mongocxx::stdx::optional<Foo> should_be_empty = to_optional_obj<Foo>(empty_optional);

        REQUIRE(!should_be_empty);

        auto should_be_filled =
            to_optional_obj<Foo>(mongocxx::stdx::optional<bsoncxx::document::value>(doc));
        REQUIRE(should_be_filled);
        if (should_be_filled) {
        }
        REQUIRE(doc_view["a"].get_int32() == should_be_filled->a);
        REQUIRE(doc_view["b"].get_int32() == should_be_filled->b);
        REQUIRE(doc_view["c"].get_int32() == should_be_filled->c);
    }

    // TODO test pipeline aggregation
    {
        coll.delete_many({});
        for (int i = 0; i < 10; i++) {
            coll.insert_one(doc_view);
        }
        Foo result_obj{10, 40, 90};

        // Set up aggregation query that sums up every field of each individual document.
        // The resulting document has the same schema, so it can be de-serialized into a Foo.
        mongocxx::pipeline stages;
        bsoncxx::builder::stream::document group_stage;
        group_stage << "_id"
                    << "a"
                    << "a" << bsoncxx::builder::stream::open_document << "$sum"
                    << "$a" << bsoncxx::builder::stream::close_document << "b"
                    << bsoncxx::builder::stream::open_document << "$sum"
                    << "$b" << bsoncxx::builder::stream::close_document << "c"
                    << bsoncxx::builder::stream::open_document << "$sum"
                    << "$c" << bsoncxx::builder::stream::close_document;
        stages.group(group_stage.view());

        auto cur = foo_coll.aggregate(stages);

        int i = 0;
        for (Foo f : cur) {
            i++;
            REQUIRE(f == result_obj);
        }
        REQUIRE(i == 1);
    }

    // test count
    {
        coll.delete_many({});
        int c0 = foo_coll.count(obj);
        REQUIRE(c0 == 0);
        coll.insert_one(doc_view);
        int c1 = foo_coll.count(obj);
        REQUIRE(c1 == 1);
        coll.insert_one(doc_view);
        int c2 = foo_coll.count(obj);
        REQUIRE(c2 == 2);
        for (int i = 0; i < 10; i++) {
            coll.insert_one(doc_view);
        }
        int c12 = foo_coll.count(obj);
        REQUIRE(c12 == 12);
        // Test that options are passed correctly
        mongocxx::options::count opts;
        opts.limit(5);
        int c5 = foo_coll.count(obj, opts);
        REQUIRE(c5 == 5);
    }

    // test delete_many
    {
        coll.delete_many({});
        for (int i = 0; i < 10; i++) {
            coll.insert_one(doc_view);
        }
        auto res = foo_coll.delete_many(obj);
        REQUIRE(res);

        if (res) {
            int c = res.value().deleted_count();
            REQUIRE(c == 10);
        }
    }

    // test delete_one
    {
        coll.delete_many({});
        coll.insert_one(doc_view);
        auto res = foo_coll.delete_one(obj);
        REQUIRE(res);

        if (res) {
            int c = res.value().deleted_count();
            REQUIRE(c == 1);
        }
    }

    // test find
    {
        coll.delete_many({});

        for (int i = 0; i < 5; i++) {
            coll.insert_one(doc_view);
            coll.insert_one(doc_2_view);
        }

        {
            auto filter = bsoncxx::from_json("{\"c\": {\"$gt\": 100}}").view();
            deserializing_cursor<Foo> cur = foo_coll.find(filter);
            int i = 0;
            for (Foo f : cur) {
                REQUIRE(f.c > 100);
                i++;
            }
            REQUIRE(i == 5);
        }

        {
            deserializing_cursor<Foo> cur = foo_coll.find(obj);
            int i = 0;
            for (Foo f : cur) {
                REQUIRE(f == obj);

                i++;
            }
            REQUIRE(i == 5);
        }
    }

    // Test find_one
    {
        coll.delete_many({});
        coll.insert_one(doc_view);

        mongocxx::stdx::optional<Foo> res = foo_coll.find_one(doc_view);
        REQUIRE(res);
        if (res) {
            Foo obj_test = res.value();
            REQUIRE(obj_test == obj);
        }
    }

    // Test find_one_and_delete
    {
        coll.delete_many({});
        coll.insert_one(doc_view);

        mongocxx::stdx::optional<Foo> res = foo_coll.find_one_and_delete(doc_view);
        REQUIRE(res);
        if (res) {
            Foo obj_test = res.value();
            REQUIRE(obj_test == obj);

            int count = coll.count(doc_view);
            REQUIRE(count == 0);
        }
    }

    // Test find_one_and_replace
    // TODO test when document could not be found
    {
        coll.delete_many({});

        Foo replacement{1, 4, 555};

        {
            coll.insert_one(doc_view);
            mongocxx::stdx::optional<Foo> res =
                foo_coll.find_one_and_replace(doc_view, replacement);
            REQUIRE(res);
            if (res) {
                Foo obj_test = res.value();
                REQUIRE(obj_test == obj);
            }
        }

        {
            coll.insert_one(doc_view);
            // This time return replacement object
            mongocxx::options::find_one_and_replace opts;
            opts.return_document(mongocxx::options::return_document::k_after);
            mongocxx::stdx::optional<Foo> res =
                foo_coll.find_one_and_replace(obj, replacement, opts);
            REQUIRE(res);
            if (res) {
                Foo obj_test = res.value();
                REQUIRE(obj_test == replacement);
            }
        }
        {
            auto res = foo_coll.find_one_and_replace(Foo{-1, -1, -1}, obj);
            REQUIRE(!res);
        }
    }

    // Test find_one_and_update()
    {
        coll.delete_many({});
        coll.insert_one(doc_view);

        std::string update_str = "{\"$inc\": {\"a\": 10}}";
        auto update_doc = bsoncxx::from_json(update_str);
        auto update_view = update_doc.view();
        auto res = foo_coll.find_one_and_update(obj, update_view);
        REQUIRE(res);
        if (res) {
            Foo obj_test = res.value();
            REQUIRE(obj_test == obj);
        }
    }

    // Test insert_one()
    {
        coll.delete_many({});

        auto res = foo_coll.insert_one(obj);
        REQUIRE(res);
        if (res) {
        }
    }

    // Test insert_many()
    {
        coll.delete_many({});
        std::vector<Foo> foo_vec;
        for (int i = 0; i < 5; i++) {
            foo_vec.push_back(Foo{0, 0, i});
        }

        {
            auto res = foo_coll.insert_many(foo_vec);
            REQUIRE(res);
            if (res) {
                int count = res.value().inserted_count();
                REQUIRE(count == 5);
            }
        }
        {
            auto res = foo_coll.insert_many(foo_vec.begin(), foo_vec.end());
            REQUIRE(res);
            if (res) {
                int count = res.value().inserted_count();
                REQUIRE(count == 5);
            }
        }
    }

    // Test replace_one()
    {
        coll.delete_many({});
        coll.insert_one(doc_view);
        Foo obj2{1, 4, 999};

        {
            auto res = foo_coll.replace_one(doc_view, obj2);
            REQUIRE(res);
            if (res) {
                int c = res.value().modified_count();
                REQUIRE(c == 1);
            }
        }

        {
            coll.insert_one(doc_view);
            auto res = foo_coll.replace_one(obj, obj2);
            REQUIRE(res);
            if (res) {
                int c = res.value().modified_count();
                REQUIRE(c == 1);
            }
        }
    }

    // Test update_many()
    {
        coll.delete_many({});
        for (int i = 0; i < 5; i++) {
            coll.insert_one(doc_view);
        }

        std::string update_str = "{\"$set\": {\"a\": 10}}";
        auto update_doc = bsoncxx::from_json(update_str);
        auto update_view = update_doc.view();

        auto res = foo_coll.update_many(obj, update_view);
        REQUIRE(res);
        if (res) {
            int c = res.value().modified_count();
            REQUIRE(c == 5);
        }
    }

    // Test update_one()
    {
        coll.delete_many({});
        // Even if there are multiple documents, update_one() should only update one of them.
        for (int i = 0; i < 5; i++) {
            coll.insert_one(doc_view);
        }

        std::string update_str = "{\"$set\": {\"a\": 10}}";
        auto update_doc = bsoncxx::from_json(update_str);
        auto update_view = update_doc.view();

        auto res = foo_coll.update_one(obj, update_view);
        REQUIRE(res);
        if (res) {
            int c = res.value().modified_count();
            REQUIRE(c == 1);
        }
    }

    // Test serializing_iterator

    coll.delete_many({});
}
