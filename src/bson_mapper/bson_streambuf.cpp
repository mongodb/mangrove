#include <cassert>
#include <cstdio>
#include <iostream>
#include <streambuf>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include "bson_streambuf.hpp"

bson_output_streambuf::bson_output_streambuf(mongocxx::collection *coll){
    this->coll = coll;
}

bson_output_streambuf::~bson_output_streambuf(){
    if (data != nullptr){
        delete[] data;
    }
}

int bson_output_streambuf::underflow(){
    return EOF;
}

int bson_output_streambuf::overflow(int ch){
    int result = EOF;
    assert(ch >= 0 && ch <= UCHAR_MAX);
    result = insert(ch);
    return result;
}

int bson_output_streambuf::insert(int ch){
    bytes_read++;

    // for first four bytes, build int32 that contains document size
    if (bytes_read <= 4){
        len += (ch << (8 * (bytes_read-1)));
    }
    // once doc size is received, allocate space.
    if (bytes_read == 4){
        // TODO perhaps fail unless 0 < len <= MAX_BSON_SIZE?
        data = new uint8_t[len];
        *(size_t *)data = len;
    }

    if (bytes_read > 4){
        data[bytes_read-1] = (uint8_t)ch;
    }

    if (bytes_read == len){
        // insert document, reset data and length
        bsoncxx::document::view v(data, len);
        coll->insert_one(v);
        delete[] data;
        data = nullptr;
        bytes_read = 0;
        len = 0;
    } else if (bytes_read > len && len >= 4){
        return EOF;
    }
    return ch;
}
