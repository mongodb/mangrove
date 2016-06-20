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

#pragma once

#include <iostream>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/stdx/optional.hpp>

#include <bson_mapper/bson_streambuf.hpp>

// A mock class for testing serialization w/o an actual BSON serializer class.
// This class is templated, but requires the given type to have primitive fields a, b, and c.
// This will be removed once cereal::BSONArchiver is merged in
template <class T>
class out_archiver_mock {
   public:
    out_archiver_mock(std::ostream& os) : _os(&os) {
    }

    void operator()(T obj) {
        bsoncxx::builder::basic::document basic_builder{};
        basic_builder.append(bsoncxx::builder::basic::kvp("a", obj.a));
        basic_builder.append(bsoncxx::builder::basic::kvp("b", obj.b));
        basic_builder.append(bsoncxx::builder::basic::kvp("c", obj.c));
        auto v = basic_builder.view();
        _os->write(reinterpret_cast<const char*>(v.data()), v.length());
    }

   private:
    std::ostream* _os;
};

// A mock class for testing de-serialization w/o an actual BSON serializer class.
// This class is templated, but requiers the given type to ahve primitize fields a, b, c.
// This will be removed once cereal::BSONArchiver is merged in
template <class T>
class in_archiver_mock {
   public:
    in_archiver_mock(std::istream& is) : _is(&is) {
    }

    void operator()(T& obj) {
        // get document size
        int size = 0;
        _is->read(reinterpret_cast<char*>(&size), 4);
        uint8_t* data = new uint8_t[size];
        // read BSON data into document view
        _is->seekg(0);
        _is->read(reinterpret_cast<char*>(data), size);
        auto doc_view = bsoncxx::document::view(data, size);
        // Fill object with fields
        obj.a = doc_view["a"].get_int32();
        obj.b = doc_view["b"].get_int32();
        obj.c = doc_view["c"].get_int32();
    }

   private:
    std::istream* _is;
};
