#ifndef _bson_streambuf_hpp
#define _bson_streambuf_hpp

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/collection.hpp>

/**
* A streambuffer that accepts bytes of BSON data,
* and inserts a document into the given collection when
* all bytes are sent over.
*/
class bson_output_streambuf : public std::streambuf
{
public:
    /**
    * Constructs a new BSON Output Streambuffer
    * that inserts documents into the given collection.
    * @param coll A pointer to a MongoDB collection.
    */
    bson_output_streambuf(mongocxx::collection *coll);

    ~bson_output_streambuf();

    /**
    * This function is called when writing to the stream and the buffer is full.
    * Since we don't define a buffer, this is called with every character.
    * @param  ch The byte of BSON to insert.
    * @return    The inserted byte, or EOF if something failed.
    */
    virtual int overflow(int ch);

    /**
    * This function always returns EOF,
    * since one should not write from an output stream.
    * @return EOF
    */
    virtual int underflow();

private:

    /**
    * Insert a byte of BSON data into the buffer.
    * The first four bytes are stored in an int, and determine the document size.
    * Then, that number of bytes (minus the first 4) are stored in the buffer.
    * When the data is complete a BSON document view is created, and inserted into
    * the collection.
    *
    * @param  ch The byte to insert.
    * @return    The byte inserted, or EOF if something failed.
    */
    int insert(int ch);
    mongocxx::collection *coll;
    uint8_t *data;
    size_t len;
    size_t bytes_read;

};

// _bson_streambuf_hpp
#endif
