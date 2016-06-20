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

#include <bson_mapper/utility.hpp>
#include <mongo_odm/deserializing_cursor.hpp>

template <class T>
class odm_collection {
   public:
    odm_collection(mongocxx::collection& c) noexcept : _coll(c) {
    }

    odm_collection(odm_collection&&) noexcept = default;
    odm_collection& operator=(odm_collection&&) noexcept = default;
    ~odm_collection() = default;

    /**
     * Returns a copy of the underlying collection.
     * @return The mongocxx::collection that this object wraps
     */
    mongocxx::collection collection() {
        return _coll;
    }

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
        const mongocxx::options::aggregate& options = mongocxx::options::aggregate()) {
        return deserializing_cursor<T>(_coll.aggregate(pipeline, options));
    }

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
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return deserializing_cursor<T>(_coll.find(filter, options));
    }

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
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return to_optional_obj<T>(_coll.find_one(filter, options));
    }

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
            mongocxx::options::find_one_and_delete()) {
        return to_optional_obj<T>(_coll.find_one_and_delete(filter, options));
    }

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
            mongocxx::options::find_one_and_replace()) {
        return to_optional_obj<T>(
            _coll.find_one_and_replace(filter, to_document(replacement), options));
    }

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
        T obj, const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return _coll.insert_one(to_document(obj), options);
    }

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
        const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return insert_many(container.begin(), container.end(), options);
    }

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
        const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return _coll.insert_many(serializing_iterator<object_iterator_type>(begin),
                                 serializing_iterator<object_iterator_type>(end), options);
    }

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
        const mongocxx::options::update& options = mongocxx::options::update()) {
        return _coll.replace_one(filter, to_document(replacement), options);
    }

   private:
    mongocxx::collection _coll;
};
