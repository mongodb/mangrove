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
 * A streambuffer that accepts one or more BSON documents as bytes of BSON data. When a doument is
 * complete, it is passed into the user-provided callback.
 * NOTE: This does not perform any validation on the BSON files,
 * and simply uses their first four bytes to judge the document length.
 */
class BSON_MAPPER_API bson_output_streambuf : public std::streambuf {
   public:
    using document_callback = std::function<void(bsoncxx::document::value)>;

    /**
    * Constructs a new BSON Output Streambuffer
    * that inserts documents into the given collection.
    * @param coll A pointer to a MongoDB collection.
    */
    bson_output_streambuf(document_callback cb);

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
    * When the data is complete a BSON document value is created, and passed to the user-provided
    * callback.
    *
    * @param  ch The byte to insert.
    * @return    The byte inserted, or EOF if something failed.
    */
    BSON_MAPPER_PRIVATE int insert(int ch);
    // This accepts a document::value and returns void.
    document_callback _cb;
    std::unique_ptr<uint8_t, void (*)(std::uint8_t*)> _data;
    size_t _len = 0;
    size_t _bytes_read = 0;
};

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
