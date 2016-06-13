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

#include <bson_mapper/config/prelude.hpp>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <streambuf>

#include <bson.h>

#include "bson_streambuf.hpp"

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

bson_output_streambuf::bson_output_streambuf(document_callback cb)
    : _cb(cb), _data(nullptr, [](uint8_t *p) {}), _bytes_read(0), _len(0) {
}

int bson_output_streambuf::underflow() {
    return EOF;
}

int bson_output_streambuf::overflow(int ch) {
    int result = EOF;
    if (ch < 0 || ch > std::numeric_limits<uint8_t>::max()) {
        throw std::invalid_argument("Character is out of bounds.");
    }
    assert(ch >= 0 && ch <= std::numeric_limits<uint8_t>::max());
    result = insert(ch);
    return result;
}

int bson_output_streambuf::insert(int ch) {
    _bytes_read++;

    // For the first four bytes, this builds int32 that contains the document size.
    if (_bytes_read <= 4) {
        _len |= (ch << (8 * (_bytes_read - 1)));
    }
    // Once the document size is received, allocate space.
    if (_bytes_read == 4) {
        if (_len > BSON_MAX_SIZE) {
            throw std::invalid_argument("BSON document length is too large.");
        }
        _data = std::unique_ptr<uint8_t, void (*)(std::uint8_t *)>(new uint8_t[_len],
                                                                   [](uint8_t *p) { delete[] p; });
        std::memcpy(_data.get(), &_len, 4);
    }

    if (_bytes_read > 4) {
        _data.get()[_bytes_read - 1] = static_cast<uint8_t>(ch);
    }

    // This creates the document from the given bytes, and calls the user-provided callback.
    if (_bytes_read == _len) {
        _cb({std::move(_data), _len});
        _bytes_read = 0;
        _len = 0;
    } else if (_bytes_read > _len && _len >= 4) {
        return EOF;
    }
    return ch;
}

char_array_streambuf::char_array_streambuf(const char *data, size_t len)
    : _begin(data), _end(data + len), _current(_begin) {
}

int char_array_streambuf::underflow() {
    if (_current == _end) {
        return EOF;
    }
    return *_current;
}

int char_array_streambuf::uflow() {
    if (_current == _end) {
        return EOF;
    }
    return *(_current++);
}

int char_array_streambuf::pbackfail(int ch) {
    // We can't put back a character if the current pointer is all the way at the beginning,
    // or if the given character does not match.
    if (_current == _begin || (ch != EOF && ch != _current[-1])) {
        return EOF;
    }
    _current--;
    return *_current;
}

std::streamsize char_array_streambuf::showmanyc() {
    return std::distance(_current, _end);
}

std::streampos char_array_streambuf::seekpos(std::streampos sp, std::ios_base::openmode which) {
    if (which & std::ios_base::in) {
        // Clamp current pointer to be within the buffer.
        _current = std::max(std::min(_begin + sp, _end), _begin);
    }
    return std::distance(_current, _end);
}

std::streampos char_array_streambuf::seekoff(std::streamoff off, std::ios_base::seekdir way,
                                             std::ios_base::openmode which) {
    // offset is relative to either the beginning, current, or end pointers.
    if (which & std::ios_base::in) {
        switch (way) {
            case std::ios_base::beg:
                _current = _begin + off;
                break;
            case std::ios_base::cur:
                _current = _current + off;
                break;
            case std::ios_base::end:
                _current = _end + off;
                break;
        }
    }
    // clamp current pointer to be within the buffer.
    _current = std::max(std::min(_current, _end), _begin);
    return std::distance(_begin, _current);
}

bson_input_streambuf::bson_input_streambuf(const bsoncxx::document::view &v)
    : char_array_streambuf(reinterpret_cast<const char *>(v.data()), v.length()) {
}

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper
