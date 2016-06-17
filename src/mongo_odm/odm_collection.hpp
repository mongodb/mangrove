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
#include <mongocxx/collection.hpp>

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
    bsoncxx::document::value doc(nullptr, 0, [](uint8_t*) {});
    bson_mapper::bson_ostream bos([&doc](bsoncxx::document::value v) { doc = std::move(v); });
    // TODO cereal::BSONOutputArchiver archiver(bos);
    out_archiver_mock<T> archive(bos);
    archive(obj);
    return doc;
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
    // TODO cereal::BSONInputArchive archive(bis);
    in_archiver_mock<T> archive(bis);
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
    // TODO cereal::BSONInputArchive archive(bis);
    in_archiver_mock<T> archive(bis);
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
mongocxx::stdx::optional<T> to_optional_obj(
    const mongocxx::stdx::optional<bsoncxx::document::value>& opt) {
    if (opt) {
        T obj = to_obj<T>(opt.value().view());
        return {obj};
    } else {
        return {};
    }
}

template <class T>
class deserializing_cursor;

template <class T>
class serializing_iterator;

template <class T>
class odm_collection {
   public:
    odm_collection(mongocxx::collection& c) noexcept;

    odm_collection(odm_collection&&) noexcept;
    odm_collection& operator=(odm_collection&&) noexcept;
    ~odm_collection();

    /**
     * Returns a copy of the underlying collection.
     * @return The mongocxx::collection that this object wraps
     */
    mongocxx::collection collection();

    ///
    /// Runs an aggregation framework pipeline against this collection, and returns the results
    /// as
    /// de-serialized object.
    ///
    /// @param pipeline
    ///   The pipeline of aggregation operations to perform.
    /// @param options
    ///   Optional arguments, see mongocxx::mongocxx::options::aggregate.
    ///
    /// @return A deserializing_cursor with the results.
    /// @throws
    ///   If the operation failed, the returned cursor will throw an exception::query
    ///   when it is iterated.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/aggregate/
    ///
    deserializing_cursor<T> aggregate(
        const mongocxx::pipeline& pipeline,
        const mongocxx::options::aggregate& options = mongocxx::options::aggregate());

    ///
    /// Counts the number of documents matching the provided filter.
    ///
    /// @param filter
    ///   The filter, as a serializable object, that documents must match in order to be
    ///   counted.
    /// @param options
    ///   Optional arguments, see mongocxx::options::count.
    ///
    /// @return The count of the documents that matched the filter.
    /// @throws exception::query if the count operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/count/
    ///
    std::int64_t count(T filter,
                       const mongocxx::options::count& options = mongocxx::options::count());

    ///
    /// Deletes all matching documents from the collection.
    ///
    /// @param filter
    ///   A serializable object representing the data to be deleted.
    /// @param options
    ///   Optional arguments, see mongocxx::mongocxx::options::delete_options.
    ///
    /// @return The optional result of performing the deletion, a result::delete_result.
    /// @throws exception::write if the delete fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/delete/
    ///
    mongocxx::stdx::optional<mongocxx::result::delete_result> delete_many(
        const T& filter,
        const mongocxx::options::delete_options& options = mongocxx::options::delete_options());

    ///
    /// Deletes a single matching document from the collection.
    ///
    /// @param filter
    ///   Serializable object representing the data to be deleted.
    /// @param options
    ///   Optional arguments, see mongocxx::mongocxx::options::delete_options.
    ///
    /// @return The optional result of performing the deletion, a result::delete_result.
    /// @throws exception::write if the delete fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/delete/
    ///
    mongocxx::stdx::optional<mongocxx::result::delete_result> delete_one(
        const T& filter,
        const mongocxx::options::delete_options& options = mongocxx::options::delete_options());

    ///
    /// Finds the documents in this collection which match the provided filter.
    ///
    /// @param filter
    ///   Document view representing a document that should match the query.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find
    ///
    /// @return Cursor with deserialized objects from the collection.
    /// @throws
    ///   If the find failed, the returned cursor will throw exception::query when it
    ///   is iterated.
    ///
    /// @see http://docs.mongodb.org/manual/core/read-operations-introduction/
    ///
    deserializing_cursor<T> find(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find());

    ///
    /// Finds the documents in this collection which match the provided object.
    ///
    /// @param filter
    ///   Serializable object representing a document that should match the query.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find
    ///
    /// @return Cursor with deserialized objects from the collection.
    /// @throws
    ///   If the find failed, the returned cursor will throw exception::query when it
    ///   is iterated.
    ///
    /// @see http://docs.mongodb.org/manual/core/read-operations-introduction/
    ///
    deserializing_cursor<T> find(
        T filter, const mongocxx::options::find& options = mongocxx::options::find());

    ///
    /// Finds a single document in this collection that match the provided filter.
    ///
    /// @param filter
    ///   Document view representing a document that should match the query.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find
    ///
    /// @return An optional object that matched the filter.
    /// @throws exception::query if the operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/core/read-operations-introduction/
    ///
    mongocxx::stdx::optional<T> find_one(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find());

    ///
    /// Finds a single document matching the filter, deletes it, and returns the original as a
    /// deserialized object.
    ///
    /// @param filter
    ///   Document view representing a document that should be deleted.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find_one_and_delete
    ///
    /// @return The deserialized object that was deleted from the database.
    /// @throws exception::write if the operation fails.
    ///
    mongocxx::stdx::optional<T> find_one_and_delete(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find_one_and_delete& options =
            mongocxx::options::find_one_and_delete());

    ///
    /// Finds a single document matching the filter, replaces it, and returns either the
    /// original
    /// or the replacement document as a deserialized object.
    ///
    /// @param filter
    ///   Document view representing a document that should be replaced.
    /// @param replacement
    ///   Serializable object representing the replacement for a matching document.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find_one_and_replace.
    ///
    /// @return The original or replaced object.
    /// @throws exception::write if the operation fails.
    ///
    /// @note
    ///   In order to pass a write concern to this, you must use the collection
    ///   level set write concern - collection::write_concern(wc).
    ///
    mongocxx::stdx::optional<T> find_one_and_replace(
        bsoncxx::document::view_or_value filter, const T& replacement,
        const mongocxx::options::find_one_and_replace& options =
            mongocxx::options::find_one_and_replace());

    ///
    /// Finds a single document matching the given serializable object, replaces it, and returns
    /// either the original or the replacement document as a deserialized object.
    ///
    /// @param filter
    ///   Serializable object representing a document that should be replaced.
    /// @param replacement
    ///   Serializable object representing the replacement for a matching document.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find_one_and_replace.
    ///
    /// @return The original or replaced object.
    /// @throws exception::write if the operation fails.
    ///
    /// @note
    ///   In order to pass a write concern to this, you must use the collection
    ///   level set write concern - collection::write_concern(wc).
    ///
    mongocxx::stdx::optional<T> find_one_and_replace(
        const T& filter, const T& replacement,
        const mongocxx::options::find_one_and_replace& options =
            mongocxx::options::find_one_and_replace());

    ///
    /// Finds a single document matching the filter, updates it, and returns either the original
    /// or the newly-updated document.
    ///
    /// @param filter
    ///   Serializable object representing a document that should be updated.
    /// @param update
    ///   Document view representing the update to apply to a matching document.
    /// @param options
    ///   Optional arguments, see mongocxx::options::find_one_and_update.
    ///
    /// @return The original or updated document as a deserialized object.
    /// @throws exception::write when the operation fails.
    ///
    /// @note
    ///   In order to pass a write concern to this, you must use the collection
    ///   level set write concern - collection::write_concern(wc).
    ///
    mongocxx::stdx::optional<T> find_one_and_update(
        const T& filter, bsoncxx::document::view_or_value update,
        const mongocxx::options::find_one_and_update& options =
            mongocxx::options::find_one_and_update());

    ///
    /// Inserts a single serializable object into the collection.
    ///
    /// // TODO how to deal w/ identifiers
    /// If the document is missing an identifier (@c _id field) one will be generated for it.
    ///
    /// @param document
    ///   The document to insert.
    /// @param options
    ///   Optional arguments, see mongocxx::options::insert.
    ///
    /// @return The result of attempting to perform the insert.
    /// @throws exception::write if the operation fails.
    ///
    mongocxx::stdx::optional<mongocxx::result::insert_one> insert_one(
        T obj, const mongocxx::options::insert& options = mongocxx::options::insert());

    ///
    /// Inserts multiple serializable objects into the collection.
    ///
    /// // TODO how to deal w/ identifiers
    /// If any of the  are missing identifiers the driver will generate them.
    ///
    /// @warning This method uses the bulk insert command to execute the insertion as opposed to
    /// the legacy OP_INSERT wire protocol message. As a result, using this method to insert
    /// many
    /// documents on MongoDB < 2.6 will be slow.
    //
    /// @tparam containter_type
    ///   The container type. Must contain an iterator that yields serializable objects.
    ///
    /// @param container
    ///   Container of serializable objects to insert.
    /// @param options
    ///   Optional arguments, see mongocxx::options::insert.
    ///
    /// @return The result of attempting to performing the insert.
    /// @throws exception::write when the operation fails.
    ///
    template <typename container_type>
    mongocxx::stdx::optional<mongocxx::result::insert_many> insert_many(
        const container_type& container,
        const mongocxx::options::insert& options = mongocxx::options::insert());

    ///
    /// Inserts multiple serializable objects into the collection.
    ///
    /// TODO how to deal w/ identifiers
    /// If any of the documents are missing /// identifiers the driver will generate them.
    ///
    /// @warning This method uses the bulk insert command to execute the insertion as opposed to
    /// the legacy OP_INSERT wire protocol message. As a result, using this method to insert
    /// many
    /// documents on MongoDB < 2.6 will be slow.
    ///
    /// @tparam object_iterator_type
    ///   The iterator type. Must meet the requirements for the input iterator concept with a
    ///   value
    ///   type that is a serializable object.
    ///
    /// @param begin
    ///   Iterator pointing to the first document to be inserted.
    /// @param end
    ///   Iterator pointing to the end of the documents to be inserted.
    /// @param options
    ///   Optional arguments, see mongocxx::options::insert.
    ///
    /// @return The result of attempting to performing the insert.
    /// @throws exception::write if the operation fails.
    ///
    /// TODO: document DocumentViewIterator concept or static assert
    template <typename object_iterator_type>
    mongocxx::stdx::optional<mongocxx::result::insert_many> insert_many(
        object_iterator_type begin, object_iterator_type end,
        const mongocxx::options::insert& options = mongocxx::options::insert());

    ///
    /// Replaces a single document matching the provided filter in this collection.
    ///
    /// @param filter
    ///   Document representing the match criteria.
    /// @param replacement
    ///   The replacement document, as a serializable object.
    /// @param options
    ///   Optional arguments, see mongocxx::options::update.
    ///
    /// @return The result of attempting to replace a document.
    /// @throws exception::write if the operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/update/
    ///
    mongocxx::stdx::optional<mongocxx::result::replace_one> replace_one(
        bsoncxx::document::view_or_value filter, const T& replacement,
        const mongocxx::options::update& options = mongocxx::options::update());

    ///
    /// Replaces a single document matching the provided serializable object in this collection.
    ///
    /// @param filter
    ///   Serializable object representing the match criteria.
    /// @param replacement
    ///   The replacement document, as a serializable object.
    /// @param options
    ///   Optional arguments, see mongocxx::options::update.
    ///
    /// @return The result of attempting to replace a document.
    /// @throws exception::write if the operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/update/
    ///
    mongocxx::stdx::optional<mongocxx::result::replace_one> replace_one(
        const T& filter, const T& replacement,
        const mongocxx::options::update& options = mongocxx::options::update());

    ///
    /// Updates multiple documents matching the provided filter in this collection.
    ///
    /// @param filter
    ///   Serializable object representing the documents to be matched.
    /// @param update
    ///   Document representing the update to be applied to matching documents.
    /// @param options
    ///   Optional arguments, see options::update.
    ///
    /// @return The result of attempting to update multiple documents.
    /// @throws exception::write if the update operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/update/
    ///
    mongocxx::stdx::optional<mongocxx::result::update> update_many(
        T filter, bsoncxx::document::view_or_value update,
        const mongocxx::options::update& options = mongocxx::options::update());

    ///
    /// Updates a single document matching the provided filter in this collection.
    ///
    /// @param filter
    ///   Serializable object representing the match criteria.
    /// @param update
    ///   Document representing the update to be applied to a matching document.
    /// @param options
    ///   Optional arguments, see options::update.
    ///
    /// @return The result of attempting to update a document.
    /// @throws exception::write if the update operation fails.
    ///
    /// @see http://docs.mongodb.org/manual/reference/command/update/
    ///
    mongocxx::stdx::optional<mongocxx::result::update> update_one(
        T filter, bsoncxx::document::view_or_value update,
        const mongocxx::options::update& options = mongocxx::options::update());

   private:
    mongocxx::collection _coll;
};

/* odm_collection implementation */

template <class T>
odm_collection<T>::odm_collection(mongocxx::collection& c) noexcept : _coll(c) {
}

template <class T>
odm_collection<T>::odm_collection(odm_collection&&) noexcept = default;

template <class T>
odm_collection<T>& odm_collection<T>::operator=(odm_collection<T>&&) noexcept = default;

template <class T>
odm_collection<T>::~odm_collection() = default;

template <class T>
mongocxx::collection odm_collection<T>::collection() {
    return _coll;
}

template <class T>
deserializing_cursor<T> odm_collection<T>::aggregate(const mongocxx::pipeline& pipeline,
                                                     const mongocxx::options::aggregate& options) {
    return deserializing_cursor<T>(_coll.aggregate(pipeline, options));
}

template <class T>
std::int64_t odm_collection<T>::count(T filter, const mongocxx::options::count& options) {
    return _coll.count(to_document(filter), options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::delete_result> odm_collection<T>::delete_many(
    const T& filter, const mongocxx::options::delete_options& options) {
    return _coll.delete_many(to_document(filter), options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::delete_result> odm_collection<T>::delete_one(
    const T& filter, const mongocxx::options::delete_options& options) {
    return _coll.delete_one(to_document(filter), options);
}

template <class T>
deserializing_cursor<T> odm_collection<T>::find(bsoncxx::document::view_or_value filter,
                                                const mongocxx::options::find& options) {
    return deserializing_cursor<T>(_coll.find(filter, options));
}

template <class T>
deserializing_cursor<T> odm_collection<T>::find(T filter, const mongocxx::options::find& options) {
    return deserializing_cursor<T>(_coll.find(to_document(filter), options));
}

template <class T>
mongocxx::stdx::optional<T> odm_collection<T>::find_one(bsoncxx::document::view_or_value filter,
                                                        const mongocxx::options::find& options) {
    return to_optional_obj<T>(_coll.find_one(filter, options));
}

template <class T>
mongocxx::stdx::optional<T> odm_collection<T>::find_one_and_delete(
    bsoncxx::document::view_or_value filter,
    const mongocxx::options::find_one_and_delete& options) {
    return to_optional_obj<T>(_coll.find_one_and_delete(filter, options));
}

template <class T>
mongocxx::stdx::optional<T> odm_collection<T>::find_one_and_replace(
    bsoncxx::document::view_or_value filter, const T& replacement,
    const mongocxx::options::find_one_and_replace& options) {
    return to_optional_obj<T>(
        _coll.find_one_and_replace(filter, to_document(replacement), options));
}

template <class T>
mongocxx::stdx::optional<T> odm_collection<T>::find_one_and_replace(
    const T& filter, const T& replacement, const mongocxx::options::find_one_and_replace& options) {
    return to_optional_obj<T>(
        _coll.find_one_and_replace(to_document(filter), to_document(replacement), options));
}

template <class T>
mongocxx::stdx::optional<T> odm_collection<T>::find_one_and_update(
    const T& filter, bsoncxx::document::view_or_value update,
    const mongocxx::options::find_one_and_update& options) {
    return to_optional_obj<T>(_coll.find_one_and_update(to_document(filter), update, options));
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::insert_one> odm_collection<T>::insert_one(
    T obj, const mongocxx::options::insert& options) {
    return _coll.insert_one(to_document(obj), options);
}

// TODO
template <class T>
template <typename container_type>
mongocxx::stdx::optional<mongocxx::result::insert_many> odm_collection<T>::insert_many(
    const container_type& container, const mongocxx::options::insert& options) {
    return insert_many(container.begin(), container.end(), options);
}

template <class T>
template <typename object_iterator_type>
mongocxx::stdx::optional<mongocxx::result::insert_many> odm_collection<T>::insert_many(
    object_iterator_type begin, object_iterator_type end,
    const mongocxx::options::insert& options) {
    return _coll.insert_many(serializing_iterator<object_iterator_type>(begin),
                             serializing_iterator<object_iterator_type>(end), options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::replace_one> odm_collection<T>::replace_one(
    bsoncxx::document::view_or_value filter, const T& replacement,
    const mongocxx::options::update& options) {
    return _coll.replace_one(filter, to_document(replacement), options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::replace_one> odm_collection<T>::replace_one(
    const T& filter, const T& replacement, const mongocxx::options::update& options) {
    return _coll.replace_one(to_document(filter), to_document(replacement), options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::update> odm_collection<T>::update_many(
    T filter, bsoncxx::document::view_or_value update, const mongocxx::options::update& options) {
    return _coll.update_many(to_document(filter), update, options);
}

template <class T>
mongocxx::stdx::optional<mongocxx::result::update> odm_collection<T>::update_one(
    T filter, bsoncxx::document::view_or_value update, const mongocxx::options::update& options) {
    return _coll.update_one(to_document(filter), update, options);
}

/**
 * A class that wraps a mongocxx::cursor. It provides an iterator that deserializes the
 * documents
 * yielded by the underlying mongocxx cursor.
 *
 * TODO what to do if some documents fail deserialization?
 */
template <class T>
class deserializing_cursor {
   public:
    deserializing_cursor(mongocxx::cursor&& c) : _c(std::move(c)) {
    }

    class iterator;

    iterator begin() {
        return iterator(_c.begin());
    }

    iterator end() {
        return iterator(_c.end());
    }

   private:
    mongocxx::cursor _c;
};

template <class T>
class deserializing_cursor<T>::iterator : public std::iterator<std::input_iterator_tag, T> {
   public:
    iterator(mongocxx::cursor::iterator ci) : _ci(ci) {
    }

    iterator(const deserializing_cursor::iterator& dsi) : _ci(dsi._ci) {
    }

    iterator& operator++() {
        _ci++;
        return *this;
    }

    void operator++(int) {
        operator++();
    }

    bool operator==(const iterator& rhs) {
        return _ci == rhs._ci;
    }

    bool operator!=(const iterator& rhs) {
        return _ci != rhs._ci;
    }

    /**
     * Returns a deserialized object that corresponds to the current document pointed to by the
     * underlying collection cursor iterator.
     */
    T operator*() {
        return to_obj<T>(*_ci);
    }

   private:
    mongocxx::cursor::iterator _ci;
};

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
    // TODO maybe use &&ci?
    serializing_iterator(Iter ci) : _ci(ci) {
    }

    serializing_iterator(const serializing_iterator& si) : _ci(si._ci) {
    }

    serializing_iterator& operator++() {
        _ci++;
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
