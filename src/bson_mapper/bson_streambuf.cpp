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

bson_output_streambuf::bson_output_streambuf(document_callback *cb)
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
        (*_cb)({std::move(_data), _len});
        _bytes_read = 0;
        _len = 0;
    } else if (_bytes_read > _len && _len >= 4) {
        return EOF;
    }
    return ch;
}

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper
