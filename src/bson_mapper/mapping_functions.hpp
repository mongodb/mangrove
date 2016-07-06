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

#include <bson_mapper/config/prelude.hpp>

#include <iostream>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/stdx/optional.hpp>

#include <bson_mapper/bson_archiver.hpp>
#include <bson_mapper/bson_streambuf.hpp>

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

/**
 * Converts a serializable object into a BSON document value
 * TODO This is not very clean and kind of inefficient. Maybe we should be passing references
 * into
 * the bson_ostream callback?
 * @tparam T   A type that is serializable to BSON using a BSONArchiver.
 * @param  obj A serializable object
 * @return     A BSON document value representing the given object.
 */
template <class T>
bsoncxx::document::value to_document(const T& obj) {
    bsoncxx::stdx::optional<bsoncxx::document::value> doc;
    bson_ostream bos([&doc](bsoncxx::document::value v) { doc = std::move(v); });
    BSONOutputArchive archive(bos);
    archive(obj);
    return doc.value();
}

/**
 * Converts a serializable object into a BSON document value in dotted notation for $set.
 * TODO This is not very clean and kind of inefficient. Maybe we should be passing references
 * into
 * the bson_ostream callback?
 * @tparam T   A type that is serializable to BSON using a BSONArchiver.
 * @param  obj A serializable object
 * @return     A BSON document value representing the given object.
 */
template <class T>
bsoncxx::document::value to_dotted_notation_document(const T& obj) {
    bsoncxx::stdx::optional<bsoncxx::document::value> doc;
    bson_ostream bos([&doc](bsoncxx::document::value v) { doc = std::move(v); });
    BSONOutputArchive archive(bos, true);
    archive(obj);
    return doc.value();
}


/**
* Converts a bsoncxx document view to an object of the templated type through deserialization.
* The object must be default-constructible.
*
* @tparam T A default-constructible type that is serializable using a BSONArchiver
* @param v A BSON document view. If the BSON document does not match the schema of type T,
* BSONArchiver will throw a corresponding exception.
* TODO what does BSONArchiver throw?
* @return Returns by value an object that corresponds to the given document view.
*/
template <class T>
T to_obj(bsoncxx::document::view v) {
    static_assert(std::is_default_constructible<T>::value,
                  "Template type must be default constructible");
    bson_mapper::bson_istream bis(v);
    bson_mapper::BSONInputArchive archive(bis);
    T obj;
    archive(obj);
    return obj;
}

/**
 * Fills a serializable object 'obj' with data from a BSON document view.
 *
 * @tparam T a type that is serializable using a BSONArchiver
 * @param v A BSON document view. If the BSON document does not match the schema of type T,
 * BSONArchiver will throw a corresponding exception.
 * TODO what does BSONArchiver throw?
 * @param obj A reference to a serializable object that will be filled with data from the gievn
 * document.
 */
template <class T>
void to_obj(bsoncxx::document::view v, T& obj) {
    bson_mapper::bson_istream bis(v);
    bson_mapper::BSONInputArchive archive(bis);
    archive(obj);
}

/*
* This function converts an stdx::optional containing a BSON document value into an
* stdx::optional
* containing a deserialized object of the templated type.
* @tparam T A type that is serializable to BSON using a BSONArchiver.
* @param opt An optional BSON document view
* @return   An optional object of type T
*/
template <class T>
bsoncxx::stdx::optional<T> to_optional_obj(
    const bsoncxx::stdx::optional<bsoncxx::document::value>& opt) {
    if (opt) {
        T obj = to_obj<T>(opt.value().view());
        return {obj};
    } else {
        return {};
    }
}

/**
 * An iterator that wraps another iterator of serializable objects, and yields BSON document
 * views
 * corresponding to those documents.
 *
 * TODO what to do if serialization fails?
 */
template <class Iter>
class serializing_iterator
    : public std::iterator<std::input_iterator_tag, bsoncxx::document::value> {
   public:
    serializing_iterator(Iter ci) : _ci(ci) {
    }

    serializing_iterator(const serializing_iterator& si) : _ci(si._ci) {
    }

    serializing_iterator& operator++() {
        ++_ci;
        return *this;
    }

    void operator++(int) {
        operator++();
    }

    bool operator==(const serializing_iterator& rhs) {
        return _ci == rhs._ci;
    }

    bool operator!=(const serializing_iterator& rhs) {
        return _ci != rhs._ci;
    }

    bsoncxx::document::value operator*() {
        return to_document(*_ci);
    }

   private:
    Iter _ci;
};

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
