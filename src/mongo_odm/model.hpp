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

#include <cereal/cereal.hpp>

#include <bsoncxx/oid.hpp>
#include <mongo_odm/config/prelude.hpp>
#include <mongo_odm/odm_collection.hpp>
#include <mongocxx/collection.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

/**
 * Helper type trait widget that helps properly forward arguments to _id constructor.
 */

template <typename... Ts>
struct Widget : public std::false_type {};

template <typename T, typename T2, typename... Ts>
struct Widget<T, T2, Ts...> : public std::is_same<T, std::decay_t<T2>> {};

/**
 * Base class for mongo_odm::model that provides _id and _id construction.
 */
template <typename IdType>
class model_odm_base {
   public:
    template <typename... Ts, typename = std::enable_if_t<!Widget<model_odm_base, Ts...>::value>>
    model_odm_base(Ts&&... ts) : _id(std::forward<Ts>(ts)...) {
    }

   protected:
    IdType _id;
};

/**
 * Specialization of mongo_odm::model base class for IdType=bsoncxx::oid that provides a default
 * constructor for _id which safely constructs a bsoncxx::oid.
 *
 * TODO: Once CXX-940 is resolved, this base class will no longer be necessary and the argument
 *       forwarding logic can take place in the mongo_odm::model class.
 */
template <>
class model_odm_base<bsoncxx::oid> {
   public:
    template <typename... Ts, typename = std::enable_if_t<!Widget<model_odm_base, Ts...>::value>>
    model_odm_base(Ts&&... ts) : _id(std::forward<Ts>(ts)...) {
    }

    model_odm_base() : _id(bsoncxx::oid::init_tag_t{}) {
    }

   protected:
    bsoncxx::oid _id;
};

template <typename T, typename IdType = bsoncxx::oid>
class model : public model_odm_base<IdType> {
   private:
// TODO: When XCode 8 is released, this can always be thread_local. Until then, the model class
//       will not be thread-safe on OS X.
#ifdef __APPLE__
    static odm_collection<T> _coll;
#else
    static thread_local odm_collection<T> _coll;
#endif

   public:
    // Forward the arguments to the constructor of IdType
    template <typename... Ts, typename = std::enable_if_t<!Widget<model, Ts...>::value>>
    model(Ts&&... ts) : model_odm_base<IdType>(std::forward<Ts>(ts)...) {
    }

    /**
     * Returns a copy of the underlying collection.
     *
     * @return A copy of the underlying mongocxx::collection that this class uses to store and
     * load instances of T.
     */
    static const mongocxx::collection collection() {
        return _coll.collection();
    }

    /**
     * Drops the underlying collection and all its contained documents from the database.
     *
     * @throws exception::operation if the operation fails.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.drop/
     */
    static void drop() {
        _coll.collection().drop();
    }

    /**
     * Sets the underlying mongocxx::collection used to store and load instances of T.
     *
     * @param coll The mongocxx::collection object to be mapped to this class.
     *
     * @warning This must be called with a new mongocxx::collection instance for every thread for
     *          the ODM to be thread-safe.
     *
     * @warning The mongocxx::client collection with which the passed-in mongocxx::collection was
     *          created must be in scope/not destructed when using the model's CRUD functionality or
     *          else the functions will segfault. If the connection goes out of scope or is
     *          destructed, this function must be called again with a fresh mongocxx::collection for
     *          the model to work properly.
     */
    static void setCollection(const mongocxx::collection& coll) {
        _coll = odm_collection<T>(coll);
    }
    static void setCollection(mongocxx::collection&& coll) {
        _coll = odm_collection<T>(std::move(coll));
    }

    /**
     * Performs an update in the database that saves the current T object instance to the
     * collection mapped to this class.
     *
     * In the terms of the CRUD specification, this uses replaceOne with the _id as the sole
     * argument to the query filter and upsert=true so that objects that aren't already in the
     * collection are automatically inserted.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.replaceOne/
     *
     * @warning This will fully replace any object in the database with an _id equal to
     *          this instance's _id.
     */
    void save() {
        auto id_match_filter = bsoncxx::builder::stream::document{}
                               << "_id" << this->_id << bsoncxx::builder::stream::finalize;

        auto update = bsoncxx::builder::stream::document{}
                      << "$set" << bson_mapper::to_document(*static_cast<T*>(this))
                      << bsoncxx::builder::stream::finalize;

        mongocxx::options::update opts{};

        opts.upsert(true);

        _coll.collection().update_one(id_match_filter.view(), update.view(), opts);

        //_coll.replace_one(id_match_filter.view(), *static_cast<T*>(this), opts);
    };

    /**
     * Deletes this object from the underlying collection.
     *
     * In the terms of the CRUD specification, this uses deleteOne with the _id as the sole
     * argument to the query filter.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.deleteOne/
     */
    void remove() {
        auto id_match_filter = bsoncxx::builder::stream::document{}
                               << "_id" << this->_id << bsoncxx::builder::stream::finalize;

        _coll.collection().delete_one(id_match_filter.view());
    }

    /**
     * Finds the documents in this collection which match the provided filter.
     *
     * @param filter
     *   Document view representing a document that should match the query.
     * @param options
     *   Optional arguments, see mongocxx::options::find
     *
     * @return Cursor with deserialized objects from the collection.
     * @throws
     *   If the find failed, the returned cursor will throw mongocxx::exception::query when it
     *   is iterated.
     *
     * @see https://docs.mongodb.com/manual/tutorial/query-documents/
     */
    static deserializing_cursor<T> find(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return _coll.find(std::move(filter), options);
    }

    /**
     * Finds a single document in this collection that matches the provided filter.
     *
     * @param filter
     *   Document view representing a document that should match the query.
     * @param options
     *   Optional arguments, see mongocxx::options::find
     *
     * @return An optional object that matched the filter.
     * @throws mongocxx::exception::query if the operation fails.
     *
     * @see https://docs.mongodb.com/manual/tutorial/query-documents/
     */
    static mongocxx::stdx::optional<T> find_one(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return _coll.find_one(std::move(filter), options);
    }
};

#ifdef __APPLE__
template <typename T, typename IdType>
odm_collection<T> model<T, IdType>::_coll;
#else
template <typename T, typename IdType>
thread_local odm_collection<T> model<T, IdType>::_coll;
#endif

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace mongo_odm
