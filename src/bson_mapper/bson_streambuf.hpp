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

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

/**
 * A streambuffer that accepts one or more BSON documents as bytes of BSON data. When a document is
 * complete, it is passed into the user-provided callback.
 * NOTE: This does not perform any validation on the BSON files,
 * and simply uses their first four bytes to judge the document length.
 */
class BSON_MAPPER_API bson_output_streambuf : public std::streambuf {
   public:
    using document_callback = std::function<void(bsoncxx::document::value)>;

    /**
    * Constructs a new BSON Output Streambuffer
    * that passes documents to the given callback function.
    * @param cb A function that takes a document::value and returns void.
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
    // A callback that accepts a document::value and returns void.
    document_callback _cb;
    std::unique_ptr<uint8_t, void (*)(std::uint8_t *)> _data;
    size_t _len = 0;
    size_t _bytes_read = 0;
};

/**
 * An ostream that writes bytes of BSON documents into a collection.
 * The stream owns its own bson_output_streambuf object, making creation and management of such
 * streams easier.
 */
class BSON_MAPPER_API bson_ostream : public std::ostream {
   public:
    bson_ostream(bson_output_streambuf::document_callback cb) : std::ostream(0), bs_buf(cb) {
        this->rdbuf(&bs_buf);
    }

   private:
    // TODO somehow set rdbuf(*sb) to private so people can't change the streambuffer?
    bson_output_streambuf bs_buf;
};

/**
 * An input streambuf that uses an existing byte array as a buffer.
 */
class BSON_MAPPER_API char_array_streambuf : public std::streambuf {
   public:
    /**
     * Create a streambuf around the given byte array.
     * The caller is responsible for maintaining the lifetime of the underlying data.
     * @param data  A pointer to the data to be read
     * @param len   The length of the data
     */
    char_array_streambuf(const char *data, size_t len);

   private:
    /**
     * This reads and returns a character from the buffer, and increments the read pointer.
     * @return [description]
     */
    virtual BSON_MAPPER_PRIVATE int underflow() override;

    // These are necessary since the buffer is not writeable, and thus we cannot call setg()
    virtual BSON_MAPPER_PRIVATE int uflow() override;
    virtual BSON_MAPPER_PRIVATE int pbackfail(int ch) override;
    virtual BSON_MAPPER_PRIVATE std::streamsize showmanyc() override;

    /**
     * This function seeks to an absolute position in the buffer.
     * @param  sp    The absolute position in the buffer
     * @param  which Which character sequence to write to. In this case, we only care about "in",
     * the input character sequence.
     * @return       The absolute (relative to start) position of the current pointer.
     */
    virtual BSON_MAPPER_PRIVATE std::streampos seekpos(
        std::streampos sp,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

    /**
     * This function seeks to a relative position in the buffer, based on an offset.
     * @param  off   The relative offset to seek to.
     * @param  way   Whether the offset is relative to the beginning, current pointer, or end of the
     * buffer.
     * @param  which Which character sequence to write to. In this case, we only care about "in",
     * the input character sequence.
     * @return       The absolute (relative to start) position of the current pointer.
     */
    virtual BSON_MAPPER_PRIVATE std::streampos seekoff(
        std::streamoff off, std::ios_base::seekdir way,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
    // Pointers to the data buffer.
    const char *const _begin;
    const char *const _end;
    const char *_current;
};

/**
 * A wrapper from char_array_streambuf, that uses the data from a BSON document view as a buffer.
 */
class BSON_MAPPER_API bson_input_streambuf : public char_array_streambuf {
   public:
    bson_input_streambuf(const bsoncxx::document::view &v);
};

/**
 * An istream that uses a BSON document as a buffer.
 * While objects of this class do not own the underlying buffer,
 * they do own the streambuf object associated with it.
 *
 * By default, istream objects do not delete their streambuffers when they are destroyed,
 * so this class allows one to create a stream without dealing with the underlying streambuf object.
 */
class BSON_MAPPER_API bson_istream : public std::istream {
   public:
    bson_istream(const bsoncxx::document::view &v) : std::istream(0), bs_buf(v) {
        this->rdbuf(&bs_buf);
    }

   private:
    // TODO somehow set rdbuf(*sb) to private so people can't change the streambuffer?
    bson_input_streambuf bs_buf;
};

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
