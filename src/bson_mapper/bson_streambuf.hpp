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

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/collection.hpp>

#include <bson_mapper/config/prelude.hpp>

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

/**
* A streambuffer that accepts bytes of BSON data,
* and inserts a document into the given collection when
* all bytes are sent over.
*/
class bson_output_streambuf : public std::streambuf {
   public:
    /**
    * Constructs a new BSON Output Streambuffer
    * that inserts documents into the given collection.
    * @param coll A pointer to a MongoDB collection.
    */
    bson_output_streambuf(mongocxx::collection coll);

    /**
    * This function is called when writing to the stream and the buffer is full.
    * Since we don't define a buffer, this is called with every character.
    * @param  ch The byte of BSON to insert.
    * @return    The inserted byte, or EOF if something failed.
    */
    int overflow(int ch) override;

    /**
    * This function always returns EOF,
    * since one should not write from an output stream.
    * @return EOF
    */
    virtual int underflow() override;

   private:
    /**
    * This function inserts a byte of BSON data into the buffer.
    * The first four bytes are stored in an int, and determine the document size.
    * Then, that number of bytes (minus the first 4) are stored in the buffer.
    * When the data is complete a BSON document view is created, and inserted into
    * the collection.
    *
    * @param  ch The byte to insert.
    * @return    The byte inserted, or EOF if something failed.
    */
    BSON_MAPPER_PRIVATE int insert(int ch);
    mongocxx::collection coll;
    std::unique_ptr<uint8_t, void (*)(std::uint8_t*)> data;
    size_t len;
    size_t bytes_read;
};

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
