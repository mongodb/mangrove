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

#include <bson_mapper/bson_archiver.hpp>
#include <bsoncxx/json.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

/**
 * Returns true if the two files are identical, false otherwise.
 */
bool file_cmp(std::string filename1, std::string filename2) {
    std::ifstream file1(filename1, std::ios_base::binary);
    std::ifstream file2(filename2, std::ios_base::binary);

    file1.seekg(0, file1.end);
    file2.seekg(0, file2.end);

    size_t file1_length = file1.tellg();
    size_t file2_length = file2.tellg();

    file1.seekg(0, file1.beg);
    file2.seekg(0, file2.beg);

    if (file1_length != file2_length) {
        return false;
    }

    char file1buf[file1_length], file2buf[file2_length];

    file1.read(file1buf, file1_length);
    file2.read(file2buf, file2_length);

    return std::memcmp(file1buf, file2buf, file1_length) == 0;
}

/**
 * Counts the number of keys in a particular document view.
 *
 * @param v
 *      the bsoncxx::document::view to count keys in.
 *
 * @return the number of keys in the view.
 */
size_t countKeys(bsoncxx::document::view v) {
    return std::distance(v.begin(), v.end());
}

struct DataA {
    int32_t x, y;
    double z;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    bool operator==(const DataA& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }
};

struct BadDataA {
    int32_t x, y;
    double z;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(x, y, z);
    }
};

struct DataB {
    int64_t a, b;
    DataA m;
    std::vector<DataA> arr;
    std::string s;
    std::chrono::system_clock::time_point tp;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(a), CEREAL_NVP(b), CEREAL_NVP(m), CEREAL_NVP(arr), CEREAL_NVP(s),
           CEREAL_NVP(tp));
    }
};

const uint8_t someBytes[] = {1, 2, 3, 4};
bsoncxx::document::value someScope = bsoncxx::from_json(R"({ "obj": {"a":1, "b":2, "c":3}})");
bsoncxx::document::value someArr =
    bsoncxx::from_json(R"([{"a":1, "b":2, "c":3}, {"a":4, "b":5, "c":6}, {"a":7, "b":8, "c":9}])");
bsoncxx::oid someOid = bsoncxx::oid("507f1f77bcf86cd799439011");

struct DataC : public bson_mapper::UnderlyingBSONDataBase {
    bsoncxx::types::b_double test_double;
    bsoncxx::types::b_utf8 test_utf8;
    bsoncxx::types::b_document test_doc;
    bsoncxx::types::b_array test_arr;
    bsoncxx::types::b_binary test_bin;
    bsoncxx::types::b_oid test_oid;
    bsoncxx::oid test_oid2;
    bsoncxx::types::b_bool test_bool;
    bsoncxx::types::b_date test_date;
    bsoncxx::types::b_int32 test_int32;
    bsoncxx::types::b_int64 test_int64;
    bsoncxx::types::b_undefined test_undefined;
    bsoncxx::types::b_null test_null;
    bsoncxx::types::b_regex test_regex;
    bsoncxx::types::b_code test_code;
    bsoncxx::types::b_symbol test_symbol;
    bsoncxx::types::b_codewscope test_codewscope;
    bsoncxx::types::b_timestamp test_timestamp;
    bsoncxx::types::b_minkey test_minkey;
    bsoncxx::types::b_maxkey test_maxkey;
    bsoncxx::types::b_dbpointer test_dbpointer;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(test_double), CEREAL_NVP(test_utf8), CEREAL_NVP(test_doc),
           CEREAL_NVP(test_arr), CEREAL_NVP(test_bin), CEREAL_NVP(test_oid), CEREAL_NVP(test_oid2),
           CEREAL_NVP(test_bool), CEREAL_NVP(test_date), CEREAL_NVP(test_int32),
           CEREAL_NVP(test_int64), CEREAL_NVP(test_undefined), CEREAL_NVP(test_null),
           CEREAL_NVP(test_regex), CEREAL_NVP(test_code), CEREAL_NVP(test_symbol),
           CEREAL_NVP(test_codewscope), CEREAL_NVP(test_timestamp), CEREAL_NVP(test_minkey),
           CEREAL_NVP(test_maxkey), CEREAL_NVP(test_dbpointer));
    }

    DataC()
        : test_double{1.4},
          test_utf8("cxx-odm"),
          test_doc{someScope.view()},
          test_bin{bsoncxx::binary_sub_type::k_binary, sizeof(someBytes), someBytes},
          test_oid{someOid},
          test_oid2(someOid),
          test_bool{true},
          test_date(std::chrono::milliseconds{1465225981}),
          test_int32{214},
          test_int64{5191872984791874},
          test_regex{"/^#?([a-f0-9]{6}|[a-f0-9]{3})$/", ""},
          test_code{R"(var obj = {a:1, b:2, c:3};
    
for (var prop in obj) {
  console.log("obj." + prop + " = " + obj[prop]);
})"},
          test_symbol{"obj"},
          test_codewscope{R"(for (var prop in obj) {
  console.log("obj." + prop + " = " + obj[prop]);
})",
                          someScope.view()},
          test_dbpointer{"dbtest", someOid} {
    }

    void resetValues() {
        test_double.value = 0.;
        test_utf8.value = "";
        test_doc.value = bsoncxx::document::view();
        test_arr.value = bsoncxx::array::view();
        test_bin.size = 0;
        test_oid.value = bsoncxx::oid(bsoncxx::oid::init_tag_t{});
        test_oid2 = bsoncxx::oid(bsoncxx::oid::init_tag_t{});
        test_bool.value = false;
        test_date.value = std::chrono::milliseconds{0};
        test_int32.value = 0;
        test_int64.value = 0;
        test_regex.regex = "";
        test_regex.options = "garbage";
        test_code.code = "";
        test_symbol.symbol = "";
        test_codewscope.code = "";
        test_codewscope.scope = bsoncxx::document::view();
        test_timestamp.increment = 0;
        test_timestamp.timestamp = 0;
        test_dbpointer.collection = "";
        test_dbpointer.value = bsoncxx::oid(bsoncxx::oid::init_tag_t{});
    }
};

TEST_CASE(
    "the BSON archiver enforces the existence of name-value pairs in output archives without "
    "segfaulting.") {
    std::ofstream os("bad_data.bson", std::ios::binary);
    bson_mapper::BSONOutputArchive archive(os);

    BadDataA a2;
    int i = 10;

    REQUIRE_THROWS(archive(a2, i));
}

TEST_CASE("the BSON archiver prevents users from loading root elements with BSON view types.") {
    {
        std::string test = "abcdefghijklmnopqrstuvwxyz1234567890";
        bsoncxx::types::b_binary b{bsoncxx::binary_sub_type::k_binary,
                                   static_cast<uint32_t>(test.size()),
                                   reinterpret_cast<const uint8_t*>(test.c_str())};

        std::ofstream os("bson_view_restriction_test.bson");
        bson_mapper::BSONOutputArchive oarchive(os);
        REQUIRE_THROWS(oarchive(CEREAL_NVP(b)));
    }
}

struct DataD : public bson_mapper::UnderlyingBSONDataBase {
    bsoncxx::types::b_binary b;
    bsoncxx::types::b_utf8 u;
    bsoncxx::types::b_document d;
    bsoncxx::types::b_array a;

    DataD()
        : b{bsoncxx::binary_sub_type::k_binary, sizeof(someBytes), someBytes},
          u("I live in the depths of an embedded document.") {
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(b), CEREAL_NVP(u), CEREAL_NVP(d), CEREAL_NVP(a));
    }
};

struct DataE {
    int32_t i;
    DataD d;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(i), CEREAL_NVP(d));
    }
};

TEST_CASE(
    "the BSON archiver successfully deserializes data with b_binary and other bsoncxx view types "
    "in an embedded document.") {
    {
        DataE test_obj;

        std::ofstream os("embedded_view_data_test.bson");
        bson_mapper::BSONOutputArchive oarchive(os);
        oarchive(test_obj);
    }

    {
        DataE test_obj;
        std::ifstream is("embedded_view_data_test.bson");
        bson_mapper::BSONInputArchive iarchive(is);
        iarchive(test_obj);

        REQUIRE(countKeys(test_obj.d.getUnderlyingBSONData()) == 4);
    }
}

TEST_CASE(
    "the BSON archiver successfully serializes and deserializes a variety of C++ classes "
    "containing BSON objects, std::vectors, and primitive types compatible with BSON.") {
    {
        std::ofstream os("data.bson", std::ios_base::binary);
        bson_mapper::BSONOutputArchive archive(os);

        DataA a1;
        DataA a2;
        DataB b1;

        std::vector<int> test_arr;

        test_arr.push_back(1);
        test_arr.push_back(2);
        test_arr.push_back(3);

        std::vector<DataA> test_obj_arr;

        a1.x = 43;
        a1.y = 229;
        a1.z = 3.14;

        a2.x = 26;
        a2.y = 32;
        a2.z = 3.4;

        test_obj_arr.push_back(a1);
        test_obj_arr.push_back(a2);

        b1.a = 517259871609285984;
        b1.b = 35781926586124;
        b1.m = a2;
        b1.arr = test_obj_arr;
        b1.s = "hello world!";
        b1.tp = std::chrono::system_clock::now();

        DataC c1;

        int i = 10;

        std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();

        archive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1);
    }

    {
        DataA a1;
        DataA a2;
        DataB b1;

        std::vector<DataA> test_obj_arr;

        DataC c1;
        c1.resetValues();

        int i;

        std::chrono::system_clock::time_point tp;

        std::ifstream is("data.bson");
        bson_mapper::BSONInputArchive archive(is);
        archive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1);

        REQUIRE(countKeys(c1.getUnderlyingBSONData()) == 21);

        std::ofstream os("data_clone.bson", std::ios_base::binary);
        bson_mapper::BSONOutputArchive oarchive(os);
        oarchive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1);
    }

    REQUIRE(file_cmp("data.bson", "data_clone.bson"));
}

TEST_CASE("the BSON archiver successfully serializes and deserializes a single C++ class.") {
    DataA a1;
    a1.x = 229;
    a1.y = 43;
    a1.z = 3.14159;

    std::ofstream os("single_test.bson");
    bson_mapper::BSONOutputArchive oarchive(os);
    oarchive(a1);
    os.close();

    DataA a2;
    std::ifstream is("single_test.bson");
    bson_mapper::BSONInputArchive iarchive(is);
    iarchive(a2);

    REQUIRE(a1 == a2);
}

TEST_CASE(
    "the BSON archiver enforces the existence of name-value pairs in input archives without "
    "segfaulting.") {
    BadDataA a1;
    DataA a2;
    DataB b1;

    std::vector<DataA> test_obj_arr;

    DataC c1;
    c1.resetValues();

    int i;

    std::chrono::system_clock::time_point tp;

    std::ifstream is("data.bson");
    bson_mapper::BSONInputArchive archive(is);

    REQUIRE_THROWS(archive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), i, a1, a2, b1, c1));
}

struct NoSerializedMembers {
    int32_t a;

    template <class Archive>
    void serialize(Archive&) {
    }
};

struct DataF {
    NoSerializedMembers m;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(m));
    }
};

/**
 * Cuts the file specified in ifilename in half and writes
 * the result to ofilename.
 */
void file_cut(std::string ifilename, std::string ofilename) {
    std::ifstream ifile(ifilename, std::ios_base::binary);

    ifile.seekg(0, ifile.end);
    size_t ifile_length = ifile.tellg();
    ifile.seekg(0, ifile.beg);

    char ifilebuf[ifile_length];
    ifile.read(ifilebuf, ifile_length);

    std::ofstream ofile(ofilename, std::ios_base::binary);
    ofile.write(ifilebuf, ifile_length / 2);
}

TEST_CASE("The BSON archiver successfully serializes a class with no members.") {
    std::ofstream os("no_members.bson", std::ios_base::binary);
    bson_mapper::BSONOutputArchive archive(os);

    DataF d;

    NoSerializedMembers m;

    archive(d);
    archive(m);
}

TEST_CASE("the BSON archiver successfully handles incomplete BSON data without segfaulting.") {
    DataA a1;
    DataA a2;
    DataB b1;

    std::vector<DataA> test_obj_arr;

    DataC c1;
    c1.resetValues();

    int i;

    std::chrono::system_clock::time_point tp;

    file_cut("data.bson", "split_data.bson");

    std::ifstream is("split_data.bson");
    bson_mapper::BSONInputArchive archive(is);
    REQUIRE_THROWS(
        archive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1));
}

struct DataG {
    bsoncxx::types::b_utf8 u;

    DataG(const char* s) : u(s) {
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(u));
    }
};

TEST_CASE(
    "the BSON archiver successfully enforces the inheritance of UnderlyingBSONDataBase for classes "
    "that contain bsoncxx view types.") {
    {
        DataG g("hello!");
        std::ofstream os("underlyingbsonbase_test.bson");
        bson_mapper::BSONOutputArchive oarchive(os);
        REQUIRE_THROWS(oarchive(g));
    }
}

TEST_CASE(
    "the BSON archiver successfully serializes and deserializes a single BSON array as a root "
    "element") {
    {
        std::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        std::ofstream os("arr_as_single_root_element.bson");
        bson_mapper::BSONOutputArchive oarchive(os);
        oarchive(CEREAL_NVP(v));
    }

    {
        std::vector<int> v;
        std::ifstream is("arr_as_single_root_element.bson");
        bson_mapper::BSONInputArchive iarchive(is);
        iarchive(CEREAL_NVP(v));
        REQUIRE(v.size() == 3);
    }
}

TEST_CASE(
    "the BSON archiver successfully serializes and deserializes a single BSON document as a root "
    "element") {
    {
        DataA a;
        a.x = 56;
        a.y = 63;
        a.z = 1.776;
        std::ofstream os("doc_as_single_root_element.bson");
        bson_mapper::BSONOutputArchive oarchive(os);
        oarchive(CEREAL_NVP(a));
    }

    {
        DataA a;
        std::ifstream is("doc_as_single_root_element.bson");
        bson_mapper::BSONInputArchive iarchive(is);
        iarchive(CEREAL_NVP(a));

        DataA a_cmp{56, 63, 1.776};
        REQUIRE(a == a_cmp);
    }
}

TEST_CASE("the BSON archiver successfully serializes embedded classes with dot notation") {
    {
        std::ofstream os("embedded_dot_notation.bson", std::ios_base::binary);
        bson_mapper::BSONOutputArchive archive(os, true);

        DataA a1, a2;
        DataB b1;

        std::vector<DataA> test_obj_arr;

        a1.x = 43;
        a1.y = 229;
        a1.z = 3.14;

        a2.x = 26;
        a2.y = 32;
        a2.z = 3.4;

        test_obj_arr.push_back(a1);
        test_obj_arr.push_back(a2);

        b1.a = 517259871609285984;
        b1.b = 35781926586124;
        b1.m = a2;
        b1.arr = test_obj_arr;
        b1.s = "hello world!";
        b1.tp = std::chrono::system_clock::now();

        archive(b1);
    }

    /*{
        DataA a1;
        DataA a2;
        DataB b1;

        std::vector<DataA> test_obj_arr;

        DataC c1;
        c1.resetValues();

        int i;

        std::chrono::system_clock::time_point tp;

        std::ifstream is("data.bson");
        bson_mapper::BSONInputArchive archive(is);
        archive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1);

        REQUIRE(countKeys(c1.getUnderlyingBSONData()) == 21);

        std::ofstream os("data_clone.bson", std::ios_base::binary);
        bson_mapper::BSONOutputArchive oarchive(os);
        oarchive(CEREAL_NVP(tp), CEREAL_NVP(test_obj_arr), CEREAL_NVP(i), a1, a2, b1, c1);
    }*/

    REQUIRE(file_cmp("data.bson", "data_clone.bson"));
}
