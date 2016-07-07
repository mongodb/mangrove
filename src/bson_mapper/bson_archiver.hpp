// Derived from https://github.com/USCiLab/cereal/blob/master/include/cereal/archives/json.hpp with
// the following license:

// Copyright (c) 2014, Randolph Voorhies, Shane Grant
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of cereal nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Modifications licensed under:

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

#include <chrono>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <cereal/cereal.hpp>

#include <bsoncxx/builder/core.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/types/value.hpp>

#include <bson_mapper/stdx/optional.hpp>

namespace bson_mapper {

/**
 * An exception class thrown when things go wrong at runtime.
 */
struct Exception : public std::runtime_error {
    explicit Exception(const std::string& desc) : std::runtime_error(desc) {
    }
    explicit Exception(const char* desc) : std::runtime_error(desc) {
    }
};

/**
 * A base class that holds a shared_ptr to the binary data for a BSON document. If a class you are
 * serializing contains any of the bsoncxx view types (b_utf8, b_document, b_array, b_binary), you
 * must have that class inherit this base so that the views don't point to deallocated memory.
 *
 * When using the BSONInputArchive to deserialize a class that inherits from this base, the data
 * will be automatically loaded into this class using setUnderlyingBSONData().
 */
class UnderlyingBSONDataBase {
   public:
    void setUnderlyingBSONData(std::shared_ptr<uint8_t> ptr, size_t size) {
        _underlyingBSONData = ptr;
        _underlyingBSONDataSize = size;
    }

    const bsoncxx::document::view getUnderlyingBSONData() {
        return bsoncxx::document::view{_underlyingBSONData.get(), _underlyingBSONDataSize};
    }

   private:
    std::shared_ptr<uint8_t> _underlyingBSONData;

    // TODO: This will be unecessary once we can read the size directly from the data
    //       in both big and little endian.
    size_t _underlyingBSONDataSize;
};

/**
 * A templated struct containing a bool value that specifies whether the
 * provided template parameter is a BSON type.
 */
template <class BsonT>
struct is_bson {
    static constexpr auto value = std::is_same<BsonT, bsoncxx::types::b_double>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_utf8>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_document>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_array>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_binary>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_oid>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_bool>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_date>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_int32>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_int64>::value ||
                                  std::is_same<BsonT, bsoncxx::oid>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_undefined>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_null>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_regex>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_code>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_symbol>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_codewscope>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_timestamp>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_minkey>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_maxkey>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_dbpointer>::value;
};

/**
 * A templated struct containing a bool value that specifies whether the
 * provided template parameter is a BSON type that contains a view.
 */
template <class BsonT>
struct is_bson_view {
    static constexpr auto value = std::is_same<BsonT, bsoncxx::types::b_utf8>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_document>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_array>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_binary>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_regex>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_dbpointer>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_code>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_symbol>::value ||
                                  std::is_same<BsonT, bsoncxx::types::b_codewscope>::value;
};

class BSONOutputArchive : public cereal::OutputArchive<BSONOutputArchive> {
    /**
    * The possible states for the BSON nodes being output by the archive.
    */
    enum class OutputNodeType { StartObject, InObject, StartArray, InArray };

    using BSONBuilder = bsoncxx::builder::core;

   public:
    /**
    * Construct a BSONOutputArchive that will output serialized classes as BSON to the provided
    * stream.
    *
    * @param stream
    *   The stream to which the archiver will output BSON data.
    *
    * @param dotNotationMode
    *   If set to true, the BSONOutputArchive will output the values in embedded documents in dot
    *   notation. This is useful when specifying the arguments to a $set field in a MongoDB update
    *   command.
    *
    *   @see https://docs.mongodb.com/manual/core/document/#embedded-documents
    *
    *   @warning Documents produced by the archiver in dotNotationMode are not compatible with the
    *            BSONInputArchiver and are only intended to be used as a way to produce the argument
    *            to $set.
    */
    BSONOutputArchive(std::ostream& stream, bool dotNotationMode = false)
        : OutputArchive<BSONOutputArchive>{this},
          _bsonBuilder{false},
          _writeStream{stream},
          _nextName{nullptr},
          _objAsRootElement{false},
          _dotNotationMode{dotNotationMode},
          _arrayNestingLevel{0} {
    }

   private:
    /**
     * Writes the current contents of the BSON document builder to the
     * output stream.
     */
    void writeDoc() {
        _writeStream.write(reinterpret_cast<const char*>(_bsonBuilder.view_document().data()),
                           _bsonBuilder.view_document().length());
        _bsonBuilder.clear();
    }

   public:
    /**
     * Checks if the most recent object written was in the root of the document.
     * If so, that document is written to the archive.
     */
    void writeDocIfRoot() {
        if (_nodeTypeStack.empty()) {
            writeDoc();
        }
    }

    /**
     * Starts a new node in the BSON output.
     *
     * The node can optionally be given a name by calling setNextName prior to
     * creating the node. Nodes only need to be started for types that are
     * objects or arrays.
     */
    void startNode(bool inheritsUnderlyingBSONDataBase) {
        writeName(true);
        _nodeTypeStack.push(OutputNodeType::StartObject);
        _curNodeInheritsUnderlyingBSONDataBase.push(inheritsUnderlyingBSONDataBase);
    }

    /**
     * Designates the most recently added node as finished, which will end
     * any objects or arrays we happen to be in.
     */
    void finishNode() {
        // Iff we ended up serializing an empty object or array, writeName
        // will never have been called - therefore the node type at the top
        // of the stack will be StartArray or StartObject - so start and
        // then immediately end the documents/array.

        // We'll also end any documents/arrays we happen to be in.

        if (_nodeTypeStack.empty()) {
            throw bson_mapper::Exception("Attempting to finish a nonexistent node.");
        }

        switch (_nodeTypeStack.top()) {
            case OutputNodeType::StartArray:
                _bsonBuilder.open_array();
                ++_arrayNestingLevel;
            // Don't break so that this array can also be closed.
            case OutputNodeType::InArray:
                _bsonBuilder.close_array();
                --_arrayNestingLevel;
                break;
            case OutputNodeType::StartObject:
                // Only consider creating a new document if we're not in dot notation mode, or if we
                // are within an array somewhere.
                if (!_dotNotationMode || _arrayNestingLevel > 0) {
                    if (_nodeTypeStack.size() > 1 || _objAsRootElement) {
                        _bsonBuilder.open_document();
                    }
                } else if (_nodeTypeStack.size() > 1) {
                    // Push back a dummy name so we don't accidentally pop an empty stack when
                    // closing this empty document.
                    _embeddedNameStack.push_back("");
                }
            // Don't break so that this document can also be closed.
            case OutputNodeType::InObject:
                // Only close the document if we are not in dot notation mode or if we are within an
                // array somewhere.
                if (!_dotNotationMode || _arrayNestingLevel > 0) {
                    if (_nodeTypeStack.size() > 1) {
                        _bsonBuilder.close_document();
                    } else if (_objAsRootElement) {
                        _bsonBuilder.close_document();
                    }
                } else if (_nodeTypeStack.size() > 1) {
                    // Pop the name of this embedded document off the stack.
                    _embeddedNameStack.pop_back();
                }
                break;
        }

        _nodeTypeStack.pop();
        _curNodeInheritsUnderlyingBSONDataBase.pop();

        // Reset the _objAsRootElement tracker bool if the stack was emptied.
        if (_nodeTypeStack.empty()) {
            _objAsRootElement = false;
        }
        writeDocIfRoot();
    }

    /**
     * Sets the name for the next node or element.
     *
     * @param name
     *    The name for the next node or element.
     */
    void setNextName(const char* name) {
        _nextName = name;
    }

    /**
     * Saves a value of type T to the current node.
     *
     * @param t
     *    A value t where T is any type that is accepted by an overload
     *    of _bsonBuilder.append(), except for bsoncxx view types.
     */
    template <class T>
    typename std::enable_if<!is_bson_view<typename std::decay<T>::type>::value>::type saveValue(
        T&& t) {
        _bsonBuilder.append(std::forward<T>(t));
    }

    /**
     * Saves a value of type T to the current node, and throws an exception if we're serializing a
     * bsoncxx view type without it being wrapped in a class that inherits UnderlyingBSONDataBase.
     *
     * @param t
     *    A value t where T is one of the bsoncxx view types.
     */
    template <class T>
    typename std::enable_if<is_bson_view<typename std::decay<T>::type>::value>::type saveValue(
        T&& t) {
        if (_curNodeInheritsUnderlyingBSONDataBase.empty() ||
            !_curNodeInheritsUnderlyingBSONDataBase.top()) {
            throw bson_mapper::Exception(
                "Cannot serialize bsoncxx view type (b_utf8, b_document, b_array, b_binary) unless "
                "that type is wrapped in a class that inherits UnderlyingBSONDataBase.");
        }
        _bsonBuilder.append(std::forward<T>(t));
    }

    /**
     * Specialization of saveValue for std::chrono::system_clock::time_point,
     * which can't be directly passed to a BSON builder without it first
     * being constructed into a bsoncxx::types::b_date.
     */
    void saveValue(std::chrono::system_clock::time_point tp) {
        _bsonBuilder.append(bsoncxx::types::b_date{tp});
    }

    /**
     * Write the name of the upcoming element and prepare object/array state.
     * Since writeName is called for every value that is output, regardless of
     * whether it has a name or not, it is the place where we will do a deferred
     * check of our node state and decide whether we are in an array or an object.
     *
     * The general workflow of saving to the BSON archive is:
     *   1. Set the name for the next node to be created, usually done by an NVP.
     *   2. Start the node.
     *   3. (if there is data to save) Write the name of the node (this function).
     *   4. (if there is data to save) Save each element of data (with saveValue).
     *   5. Finish the node.
     *
     * @param isNewNode
     *  Specifies whether this is writing the name of a new node. If this is set to true,
     *  and we happen to be in the root node no name will be enforced since root level documents
     *  have no name.
     */
    void writeName(bool isNewNode = false) {
        if (!_nodeTypeStack.empty()) {
            const auto& topType = _nodeTypeStack.top();

            // Start up either an object or an array, depending on state.
            if (topType == OutputNodeType::StartArray) {
                _bsonBuilder.open_array();
                ++_arrayNestingLevel;
                _nodeTypeStack.top() = OutputNodeType::InArray;
            } else if (topType == OutputNodeType::StartObject) {
                _nodeTypeStack.top() = OutputNodeType::InObject;
                // Only open a document if this document is not in the root node or this is a root
                // element.
                if (_nodeTypeStack.size() > 1 || _objAsRootElement) {
                    if (_dotNotationMode && _arrayNestingLevel == 0) {
                        _embeddedNameStack.push_back(_nextPotentialNodeName);
                    } else {
                        _bsonBuilder.open_document();
                    }
                }
            }

            // Elements in arrays do not have names.
            if (topType == OutputNodeType::InArray) return;
        }

        if (!_nextName && _nodeTypeStack.empty() && isNewNode) {
            // This is a document at the root node with no name.
            // Do nothing since no name is expected for non root-element documents in root.
            return;
        } else if (!_nextName) {
            // Enforce the existence of a name-value pair if this is not a node in root,
            // or this in an element in root.
            throw bson_mapper::Exception("Missing a name for current node or element.");
        } else {
            // Set the key of this element to the name stored by the archiver.
            if (!_dotNotationMode || _embeddedNameStack.empty() || _arrayNestingLevel > 0) {
                _bsonBuilder.key_view(_nextName);
            } else {
                // If we are in dot notation mode and we're not nested in array, build the name of
                // this key.
                std::stringstream key;
                for (const auto& name : _embeddedNameStack) {
                    key << name << ".";
                }
                key << _nextName;
                _bsonBuilder.key_owned(std::move(key.str()));
            }

            // Save the name of this key in case it is the name of an embedded document.
            _nextPotentialNodeName = _nextName;

            // Reset the name of the next key.
            _nextName = nullptr;

            // If this a named node in the root node, set up this node as a root element.
            if (_nodeTypeStack.empty() && isNewNode) {
                _objAsRootElement = true;
            }
        }
    }

    /**
     * Designates that the current node should be output as an array, not an
     * object.
     */
    void makeArray() {
        _nodeTypeStack.top() = OutputNodeType::StartArray;
    }

   private:
    // The BSONCXX builder for this archive.
    BSONBuilder _bsonBuilder;

    // The stream to which to write the BSON archive.
    std::ostream& _writeStream;

    // The name of the next element to be added to the archive.
    char const* _nextName;

    // The name of the last element added to the archive, which may potentially be the name of the
    // next embedded document. This is stored to support dot notation mode.
    char const* _nextPotentialNodeName;

    // A stack maintaining the state of the nodes currently being written.
    std::stack<OutputNodeType> _nodeTypeStack;

    // Stack maintaining whether or not the node we're serializing inherits from the underlying BSON
    // data base class required for classes that use bsoncxx view types.
    std::stack<bool> _curNodeInheritsUnderlyingBSONDataBase;

    // A stack of names maintained in dot notation mode that keeps track of the names of the
    // embedded documents we are nested in.
    std::vector<std::string> _embeddedNameStack;

    // Boolean value that tracks whether or not the current root BSON document or array is being
    // treated as a root element.
    bool _objAsRootElement;

    // Bool tracking whether or not the resulting document will specify the values of embedded
    // fields in dot notation so they can be used as an argument to $set in an update operation.
    bool _dotNotationMode;

    // Byte-sized unsigned int that tracks how many arrays we are currently nested in. If this is
    // equal to 0, we are not in an array and can write keys in dot notation.
    uint8_t _arrayNestingLevel;

};  // BSONOutputArchive

class BSONInputArchive : public cereal::InputArchive<BSONInputArchive> {
    enum class InputNodeType { InObject, InEmbeddedObject, InEmbeddedArray, InRootElement };

   public:
    /**
    * Construct a BSONInputArchive from an input stream of BSON data.
    *
    * @param stream
    *    The stream from which to read BSON data.
    */
    BSONInputArchive(std::istream& stream)
        : InputArchive<BSONInputArchive>(this),
          _nextName(nullptr),
          _readStream(stream),
          _readFirstDoc(false) {
    }

   private:
    /**
     * Reads the next BSON document from the istream. This should be called whenever
     * we are starting to load in a root element or root node.
     */
    void readNextDoc() {
        // Determine the size of the BSON document in bytes.
        // TODO: Only works on little endian.
        int32_t docsize;
        char docsize_buf[sizeof(docsize)];
        _readStream.read(docsize_buf, sizeof(docsize));
        std::memcpy(&docsize, docsize_buf, sizeof(docsize));

        // Throw an exception if the end of the stream is prematurely reached.
        if (_readStream.eof() || !_readStream || docsize < 5) {
            throw bson_mapper::Exception("No more data in BSONInputArchive stream.");
        }

        // Create a shared buffer to store the BSON document.
        _curBsonData =
            std::shared_ptr<uint8_t>{new uint8_t[docsize], [](uint8_t* p) { delete[] p; }};

        // Read the BSON data from the stream into the buffer.
        std::memcpy(_curBsonData.get(), docsize_buf, sizeof(docsize));
        _readStream.read(reinterpret_cast<char*>(_curBsonData.get() + sizeof(docsize)),
                         docsize - sizeof(docsize));

        // Make sure there were no errors reading the BSON data.
        if (_readStream.eof() || !_readStream) {
            throw bson_mapper::Exception("No more data in BSONInputArchive stream.");
        }

        // Store the BSON data of the document in a view that we can access.
        _curBsonDoc = bsoncxx::document::view{_curBsonData.get(), static_cast<size_t>(docsize)};
        _curBsonDataSize = docsize;

        // Specify that we've read a document.
        _readFirstDoc = true;
    }

    /**
     * Searches for the next BSON element to be retrieved and loaded.
     *
     * @return The element with _nextName as a key, or if the current node is an array, the next
     *         element in that array.
     */
    inline bsoncxx::types::value search() {
        // If our search result is cached, return the cached result instead of repeating the search.
        if (_cachedSearchResult) {
            auto val = *_cachedSearchResult;
            _cachedSearchResult = stdx::nullopt;
            return val;
        }

        // Make sure the node stack is not empty.
        if (_nodeTypeStack.empty()) {
            throw bson_mapper::Exception("Cannot search for element if not in node.");
        }

        // Check to make sure a document has been read.
        if (!_readFirstDoc) {
            throw bson_mapper::Exception(
                "Error in element search, never loaded BSON data from file.");
        }

        if (_nextName) {
            // Reset _nextName
            const char* nextName = _nextName;
            _nextName = nullptr;

            if (_nodeTypeStack.top() == InputNodeType::InObject ||
                _nodeTypeStack.top() == InputNodeType::InRootElement) {
                // If we're in an object in the Root (InObject),
                // look for the key in the current BSON view.
                const auto& elemFromDoc = _curBsonDoc[nextName];
                if (elemFromDoc) {
                    return elemFromDoc.get_value();
                }
            } else if (_nodeTypeStack.top() == InputNodeType::InEmbeddedObject) {
                // If we're in an embedded object, look for the key in the object
                // at the top of the embedded object stack.
                const auto& elemFromDoc = _embeddedBsonDocStack.top()[nextName];
                if (elemFromDoc) {
                    return elemFromDoc.get_value();
                }
            }

            // Provide an exception with an error message if the key is not found.
            std::stringstream error_msg;
            error_msg << "No element found with the key ";
            error_msg << nextName;
            error_msg << ".";
            throw bson_mapper::Exception(error_msg.str());

        } else if (_nodeTypeStack.top() == InputNodeType::InEmbeddedArray) {
            // If we're in an array (InEmbeddedArray), retrieve an element from
            // the array iterator at the top of the stack, and increment it for
            // the next retrieval.
            auto& iter = _embeddedBsonArrayIteratorStack.top();
            const auto elemFromArr = *iter;
            ++iter;

            if (elemFromArr) {
                return elemFromArr.get_value();
            }

            throw bson_mapper::Exception(
                "Invalid element found in array, or array is out of bounds.");
        }

        throw bson_mapper::Exception("Missing name for element search.");
    }

   public:
    /**
     * Checks if the next invocation of search() will yield a value. Used to check if a particular
     * optional element, embedded document, or embedded array exists.
     *
     * If search() would indeed return a value, it is cached here so that the logic in search()
     * will not need to be repeated.
     *
     * @return true if the next invocation of search with the current _nextName yields a value,
     *         false otherwise.
     */
    bool willSearchYieldValue() {
        // Make sure the node stack is not empty.
        if (_nodeTypeStack.empty()) {
            throw bson_mapper::Exception("Cannot search for element if not in node.");
        }

        // Check to make sure a document has been read.
        if (!_readFirstDoc) {
            throw bson_mapper::Exception(
                "Error in element search, never loaded BSON data from file.");
        }

        if (_nodeTypeStack.top() == InputNodeType::InEmbeddedArray) {
            throw cereal::Exception(
                "Should not be checking if search() will yield next value from an embedded array.");
        }

        if (_nextName) {
            const char* nextName = _nextName;
            _nextName = nullptr;

            bsoncxx::document::element val{};

            if (_nodeTypeStack.top() == InputNodeType::InObject ||
                _nodeTypeStack.top() == InputNodeType::InRootElement) {
                val = _curBsonDoc[nextName];

            } else {
                val = _embeddedBsonDocStack.top()[nextName];
            }

            if (val) {
                _cachedSearchResult = val.get_value();
                return true;
            }
        }

        _cachedSearchResult = stdx::nullopt;
        return false;
    }

    /**
     * Pushes a root element on the node stack if we're in root.
     *
     * @return true if a root element was created, false otherwise.
     */
    bool startRootElementIfRoot() {
        if (_nodeTypeStack.empty()) {
            readNextDoc();
            _nodeTypeStack.push(InputNodeType::InRootElement);
            return true;
        }
        return false;
    }

    /**
     * Pops the node stack and iterates to the next BSON view if the
     * top of the stack specifies that we are in a root element.
     */
    void finishRootElementIfRootElement() {
        if (!_nodeTypeStack.empty() && _nodeTypeStack.top() == InputNodeType::InRootElement) {
            _nodeTypeStack.pop();
        }
    }

    /**
    * Starts a new node, and update the stacks so that
    * we fetch the correct data when calling search().
    */
    void startNode() {
        if (_nodeTypeStack.empty()) {
            // If we are in the root node, read in the next document from the stream,
            // and update the state of the node we're currently in.
            readNextDoc();

            // If there is a name, the new node is loading a BSON document or array as a root
            // element.
            if (_nextName) {
                _nodeTypeStack.push(InputNodeType::InRootElement);
                auto newNode = search();

                if (newNode.type() == bsoncxx::type::k_array) {
                    _embeddedBsonArrayStack.push(newNode.get_array().value);
                    _embeddedBsonArrayIteratorStack.push(_embeddedBsonArrayStack.top().begin());
                    _nodeTypeStack.push(InputNodeType::InEmbeddedArray);
                } else if (newNode.type() == bsoncxx::type::k_document) {
                    _embeddedBsonDocStack.push(newNode.get_document().value);
                    _nodeTypeStack.push(InputNodeType::InEmbeddedObject);
                } else {
                    throw bson_mapper::Exception("Node requested is neither document nor array.");
                }
            } else {
                _nodeTypeStack.push(InputNodeType::InObject);
            }
        } else {
            // If we're not in the root node, match the next key to an embedded document
            // or array.
            // From the BSON document we're currently in, fetch the value associated
            // with this node and update the relevant stacks.
            auto newNode = search();

            if (newNode.type() == bsoncxx::type::k_document) {
                _embeddedBsonDocStack.push(newNode.get_document().value);
                _nodeTypeStack.push(InputNodeType::InEmbeddedObject);
            } else if (newNode.type() == bsoncxx::type::k_array) {
                _embeddedBsonArrayStack.push(newNode.get_array().value);
                _embeddedBsonArrayIteratorStack.push(_embeddedBsonArrayStack.top().begin());
                _nodeTypeStack.push(InputNodeType::InEmbeddedArray);
            } else {
                throw bson_mapper::Exception("Node requested is neither document nor array.");
            }
        }
    }

    /**
     * Finishes the most recently started node by popping relevant stacks
     * and, if necessary, iterating to the next root BSON document.
     */
    void finishNode() {
        // If we're in an embedded object or array, pop it from its respective
        // stack(s).
        if (_nodeTypeStack.top() == InputNodeType::InEmbeddedObject) {
            _embeddedBsonDocStack.pop();
        } else if (_nodeTypeStack.top() == InputNodeType::InEmbeddedArray) {
            _embeddedBsonArrayStack.pop();
            _embeddedBsonArrayIteratorStack.pop();
        }

        // Pop the node type from the stack.
        _nodeTypeStack.pop();

        if (!_nodeTypeStack.empty() && _nodeTypeStack.top() == InputNodeType::InRootElement) {
            _nodeTypeStack.pop();
        }
    }

    /**
     * Sets the name for the next node created with startNode
     *
     * @param name
     *  The name of the next node
     */
    void setNextName(const char* name) {
        _nextName = name;
    }

   private:
    /**
     * Throws an exception if the type of v is not the specified type t.
     */
    inline void assert_type(const bsoncxx::types::value& v, bsoncxx::type t) {
        if (v.type() != t) {
            throw bson_mapper::Exception("Type mismatch when loading values.");
        }
    }

   public:
/**
 * Loads a BSON value from the current node into a bsoncxx::types variable.
 *
 * @param val
 *    The bsoncxx::types typed variable into which the BSON value will be loaded.
 */
#define MONGO_ODM_BSON_LOAD_VALUE_FUNC(btype)           \
    void loadValue(bsoncxx::types::b_##btype& val) {    \
        auto bsonVal = search();                        \
        assert_type(bsonVal, bsoncxx::type::k_##btype); \
        val = bsonVal.get_##btype();                    \
    }

    // Invokes the macro for all non-deprecated, non-internal
    // BSON types supported by bsoncxx
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(double)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(utf8)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(document)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(array)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(binary)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(oid)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(bool)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(date)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(int32)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(int64)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(undefined)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(null)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(regex)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(code)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(symbol)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(codewscope)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(timestamp)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(minkey)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(maxkey)
    MONGO_ODM_BSON_LOAD_VALUE_FUNC(dbpointer)

#undef MONGO_ODM_BSON_LOAD_VALUE_FUNC

/**
 * Loads a BSON value from the current node into a non-bsoncxx::types variable.
 *
 * @param val
 *    The non-bsoncxx::types variable into which the value will be loaded.
 */
#define MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(cxxtype, btype) \
    void loadValue(cxxtype& val) {                         \
        auto bsonVal = search();                           \
        assert_type(bsonVal, bsoncxx::type::k_##btype);    \
        val = bsonVal.get_##btype().value;                 \
    }

    MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(bsoncxx::oid, oid)
    MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(bool, bool)
    MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(std::int32_t, int32)
    MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(std::int64_t, int64)
    MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC(double, double)

#undef MONGO_ODM_NON_BSON_LOAD_VALUE_FUNC

    /**
     * Loads a BSON datetime from the current node and puts it into
     * a std::chrono::system_clock::time_point.
     *
     * @param val
     *    The time_point variable into which the datetime will be loaded.
     */
    void loadValue(std::chrono::system_clock::time_point& val) {
        auto bsonVal = search();
        assert_type(bsonVal, bsoncxx::type::k_date);
        val = std::chrono::system_clock::time_point(
            std::chrono::milliseconds{bsonVal.get_date().value});
    }

    /**
     * Loads a BSON UTF-8 value from the current node and puts it into a std::string.
     *
     * @param val
     *    The std::string variable into which the UTF-8 will be loaded.
     */
    void loadValue(std::string& val) {
        auto bsonVal = search();
        assert_type(bsonVal, bsoncxx::type::k_utf8);
        val = bsonVal.get_utf8().value.to_string();
    }

    /**
     * Loads the size for a SizeTag, which is used by Cereal to determine how many
     * elements to put into a container such as a std::vector.
     *
     * @param size
     *  A reference to the size value that will be set to the size of the array at the top of
     * the stack.
     */
    void loadSize(cereal::size_type& size) {
        if (!_nodeTypeStack.empty() && _nodeTypeStack.top() != InputNodeType::InEmbeddedArray) {
            throw bson_mapper::Exception("Requesting a size tag when not in an array.");
        }
        size = std::distance(_embeddedBsonArrayStack.top().begin(),
                             _embeddedBsonArrayStack.top().end());
    }

    /**
     * Returns a shared pointer to the underlying data of the current node,
     * loading the size in bytes in a size_t reference argument.
     */
    void loadUnderlyingDataForCurrentNode(UnderlyingBSONDataBase& underlyingData) {
        if (_nodeTypeStack.empty()) {
            throw bson_mapper::Exception("Cannot get data; not currently in a node.");
        }

        switch (_nodeTypeStack.top()) {
            case InputNodeType::InObject:
            case InputNodeType::InRootElement:
                underlyingData.setUnderlyingBSONData(std::shared_ptr<uint8_t>(_curBsonData),
                                                     _curBsonDataSize);
                return;
            case InputNodeType::InEmbeddedObject:
                // Use the aliasing constructor of shared_ptr to build a reference pointing to the
                // embedded document while reference counting the BSON doc as a whole.
                underlyingData.setUnderlyingBSONData(
                    std::shared_ptr<uint8_t>(
                        _curBsonData, const_cast<uint8_t*>(_embeddedBsonDocStack.top().data())),
                    _embeddedBsonDocStack.top().length());
                return;
            case InputNodeType::InEmbeddedArray:
                throw bson_mapper::Exception(
                    "Underlying BSON data does not support array views. Wrap the b_array, or your "
                    "container holding other bsoncxx view types in a class that inherits "
                    "bson_mapper::UnderlyingBSONDataBase");
                return;
        }
    }

   private:
    // The key name of the next element being searched.
    const char* _nextName;

    // The stream of BSON being read.
    std::istream& _readStream;

    // Bool that tracks whether or not a document has been read from the stream.
    bool _readFirstDoc;

    // Cache for the next search result if willSearchYieldValue() returns true.
    stdx::optional<bsoncxx::types::value> _cachedSearchResult;

    // The current root BSON document being viewed.
    std::shared_ptr<uint8_t> _curBsonData;
    size_t _curBsonDataSize;
    bsoncxx::document::view _curBsonDoc;

    // Stack maintaining views of embedded BSON documents.
    std::stack<bsoncxx::document::view> _embeddedBsonDocStack;

    // Stacks maintaining views of embedded BSON arrays, as well as their
    // iterators.
    std::stack<bsoncxx::array::view> _embeddedBsonArrayStack;
    std::stack<bsoncxx::array::view::iterator> _embeddedBsonArrayIteratorStack;

    // A stack maintaining the state of the node currently being worked on.
    std::stack<InputNodeType> _nodeTypeStack;

};  // BSONInputArchive

// ######################################################################
// BSONArchive prologue and epilogue functions
// ######################################################################

// ######################################################################
// Prologue for NVPs for BSON output archives
// NVPs do not start or finish nodes - they just set up the names
template <class T>
inline void prologue(BSONOutputArchive&, cereal::NameValuePair<T> const&) {
}

// Prologue for NVPs for BSON input archives
template <class T>
inline void prologue(BSONInputArchive&, cereal::NameValuePair<T> const&) {
}

// ######################################################################
// Epilogue for NVPs for BSON output archives
// NVPs do not start or finish nodes - they just set up the names
template <class T>
inline void epilogue(BSONOutputArchive&, cereal::NameValuePair<T> const&) {
}

// Epilogue for NVPs for BSON input archives
// NVPs do not start or finish nodes - they just set up the names
template <class T>
inline void epilogue(BSONInputArchive&, cereal::NameValuePair<T> const&) {
}

// ######################################################################
// Prologue for stdx::optionals for BSON output archives
// stdx::optionals do not start or finish nodes - they just set up the names
template <class T>
inline void prologue(BSONOutputArchive&, stdx::optional<T> const&) {
}

// Prologue for stdx::optionals for BSON input archives
template <class T>
inline void prologue(BSONInputArchive&, stdx::optional<T> const&) {
}

// ######################################################################
// Epilogue for stdx::optionals for BSON output archives
// stdx::optionals do not start or finish nodes - they just set up the names
template <class T>
inline void epilogue(BSONOutputArchive&, stdx::optional<T> const&) {
}

// Epilogue for stdx::optionals for BSON input archives
// stdx::optionals do not start or finish nodes - they just set up the names
template <class T>
inline void epilogue(BSONInputArchive&, stdx::optional<T> const&) {
}

// ######################################################################
// Prologue for SizeTags for BSON output archives
// SizeTags are strictly ignored for BSON, they just indicate
// that the current node should be made into an array
template <class T>
inline void prologue(BSONOutputArchive& ar, cereal::SizeTag<T> const&) {
    ar.makeArray();
}

// Prologue for SizeTags for BSON input archives
template <class T>
inline void prologue(BSONInputArchive&, cereal::SizeTag<T> const&) {
}

// ######################################################################
// Epilogue for SizeTags for BSON output archives
// SizeTags are strictly ignored for BSON
template <class T>
inline void epilogue(BSONOutputArchive&, cereal::SizeTag<T> const&) {
}

// Epilogue for SizeTags for BSON input archives
template <class T>
inline void epilogue(BSONInputArchive&, cereal::SizeTag<T> const&) {
}

// ######################################################################
// Prologue and Epilogue for BSON types, which should not be confused
// as objects or arrays

template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void prologue(BSONOutputArchive& ar, BsonT const&) {
    ar.writeName();
}

template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void epilogue(BSONOutputArchive& ar, BsonT const&) {
    ar.writeDocIfRoot();
}

template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void prologue(BSONInputArchive& ar, BsonT const&) {
    if (ar.startRootElementIfRoot()) {
        if (is_bson_view<BsonT>::value) {
            throw bson_mapper::Exception(
                "Cannot deserialize a BSON view type into a root element. The BSON view type must "
                "be wrapped in a class that inherits bson_mapper::UnderlyingBSONDataBase");
        }
    }
}

template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void epilogue(BSONInputArchive& ar, BsonT const&) {
    ar.finishRootElementIfRootElement();
}

// ######################################################################
// Prologue for all other types for BSON output archives (except minimal types)
// Starts a new node, named either automatically or by some NVP,
// that may be given data by the type about to be archived
// Minimal types do not start or finish nodes.

// Prologue for all types which inherit from UnderlyingBSONDataBase for BSON output archives.
template <class T, cereal::traits::EnableIf<std::is_base_of<UnderlyingBSONDataBase, T>::value> =
                       cereal::traits::sfinae>
inline void prologue(BSONOutputArchive& ar, T const&) {
    ar.startNode(true);
}

template <class T,
          cereal::traits::DisableIf<
              std::is_arithmetic<T>::value ||
              cereal::traits::has_minimal_base_class_serialization<
                  T, cereal::traits::has_minimal_output_serialization, BSONOutputArchive>::value ||
              cereal::traits::has_minimal_output_serialization<T, BSONOutputArchive>::value ||
              is_bson<T>::value || std::is_same<T, std::chrono::system_clock::time_point>::value ||
              std::is_base_of<UnderlyingBSONDataBase, T>::value> = cereal::traits::sfinae>
inline void prologue(BSONOutputArchive& ar, T const&) {
    ar.startNode(false);
}

// Prologue for all types which inherit from UnderlyingBSONDataBase for BSON input archives.
template <class T, cereal::traits::EnableIf<std::is_base_of<UnderlyingBSONDataBase, T>::value> =
                       cereal::traits::sfinae>
inline void prologue(BSONInputArchive& ar, T& obj) {
    ar.startNode();
    ar.loadUnderlyingDataForCurrentNode(obj);
}

// Prologue for all other types for BSON input archives.
template <class T,
          cereal::traits::DisableIf<
              std::is_arithmetic<T>::value ||
              cereal::traits::has_minimal_base_class_serialization<
                  T, cereal::traits::has_minimal_input_serialization, BSONInputArchive>::value ||
              cereal::traits::has_minimal_input_serialization<T, BSONInputArchive>::value ||
              is_bson<T>::value || std::is_same<T, std::chrono::system_clock::time_point>::value ||
              std::is_base_of<UnderlyingBSONDataBase, T>::value> = cereal::traits::sfinae>
inline void prologue(BSONInputArchive& ar, T const&) {
    ar.startNode();
}

// ######################################################################
// Epilogue for all other types other for BSON output archives (except minimal
// types
// Finishes the node created in the prologue

// Minimal types do not start or finish nodes
template <class T,
          cereal::traits::DisableIf<
              std::is_arithmetic<T>::value ||
              cereal::traits::has_minimal_base_class_serialization<
                  T, cereal::traits::has_minimal_output_serialization, BSONOutputArchive>::value ||
              cereal::traits::has_minimal_output_serialization<T, BSONOutputArchive>::value ||
              is_bson<T>::value || std::is_same<T, std::chrono::system_clock::time_point>::value> =
              cereal::traits::sfinae>
inline void epilogue(BSONOutputArchive& ar, T const&) {
    ar.finishNode();
}

// Epilogue for all other types other for BSON input archives
template <class T,
          cereal::traits::DisableIf<
              std::is_arithmetic<T>::value ||
              cereal::traits::has_minimal_base_class_serialization<
                  T, cereal::traits::has_minimal_input_serialization, BSONInputArchive>::value ||
              cereal::traits::has_minimal_input_serialization<T, BSONInputArchive>::value ||
              is_bson<T>::value || std::is_same<T, std::chrono::system_clock::time_point>::value> =
              cereal::traits::sfinae>
inline void epilogue(BSONInputArchive& ar, T const&) {
    ar.finishNode();
}

// ######################################################################
// Prologue for arithmetic types for BSON output archives
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void prologue(BSONOutputArchive& ar, T const&) {
    ar.writeName();
}

// Prologue for arithmetic types for BSON input archives
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void prologue(BSONInputArchive& ar, T const&) {
    ar.startRootElementIfRoot();
}

// ######################################################################
// Epilogue for arithmetic types for BSON output archives
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void epilogue(BSONOutputArchive& ar, T const&) {
    ar.writeDocIfRoot();
}

// Epilogue for arithmetic types for BSON input archives
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void epilogue(BSONInputArchive& ar, T const&) {
    ar.finishRootElementIfRootElement();
}

// ######################################################################
// Prologue for strings for BSON output archives
template <class CharT, class Traits, class Alloc>
inline void prologue(BSONOutputArchive& ar, std::basic_string<CharT, Traits, Alloc> const&) {
    ar.writeName();
}

// Prologue for strings for BSON input archives
template <class CharT, class Traits, class Alloc>
inline void prologue(BSONInputArchive& ar, std::basic_string<CharT, Traits, Alloc> const&) {
    ar.startRootElementIfRoot();
}

// ######################################################################
// Epilogue for strings for BSON output archives
template <class CharT, class Traits, class Alloc>
inline void epilogue(BSONOutputArchive& ar, std::basic_string<CharT, Traits, Alloc> const&) {
    ar.writeDocIfRoot();
}

// Epilogue for strings for BSON output archives
template <class CharT, class Traits, class Alloc>
inline void epilogue(BSONInputArchive& ar, std::basic_string<CharT, Traits, Alloc> const&) {
    ar.finishRootElementIfRootElement();
}

// ######################################################################
// Common BSONArchive serialization functions
// ######################################################################
// Serializing NVP types to BSON
template <class T>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive& ar, cereal::NameValuePair<T> const& t) {
    ar.setNextName(t.name);
    ar(t.value);
}

// Loading NVP types from BSON
template <class T>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar, cereal::NameValuePair<T>& t) {
    ar.setNextName(t.name);
    ar(t.value);
}

// Serializing stdx::optional types to BSON
template <class T>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive& ar, stdx::optional<T> const& t) {
    // Only serialize the value if it is present. Otherwise do nothing.
    if (t) {
        ar(*t);
    }
}

// Loading stdx::optional types from BSON
//
// Note that if a key for an optional is not present in the BSON from which we are loading values,
// the BSONInputArchive will overwrite what was in the destination stdx::optional with a
// stdx::nullopt, even if there was already a value.
//
template <class T,
          cereal::traits::DisableIf<is_bson<T>::value && !std::is_default_constructible<T>::value> =
              cereal::traits::sfinae>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar, stdx::optional<T>& t) {
    if (ar.willSearchYieldValue()) {
        T value;
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}

// Overloads for the stdx::optional types wrapping non-default-constructible BSON b_ types.
// TODO: Once CXX-966 is resolved (making all BSON b_ types default constructible) these overloads
//       will no longer be necessary.
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_utf8>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_utf8 value{""};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_date>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_date value{std::chrono::system_clock::time_point{}};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_regex>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_regex value{"", ""};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_code>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_code value{""};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_codewscope>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_codewscope value{"", bsoncxx::document::view()};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      stdx::optional<bsoncxx::types::b_symbol>& t) {
    if (ar.willSearchYieldValue()) {
        bsoncxx::types::b_symbol value{""};
        ar(value);
        t.emplace(value);
    } else {
        t = stdx::nullopt;
    }
}

// Saving for arithmetic to BSON
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive& ar, T const& t) {
    ar.saveValue(t);
}

// Loading arithmetic from BSON
template <class T, cereal::traits::EnableIf<std::is_arithmetic<T>::value> = cereal::traits::sfinae>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar, T& t) {
    ar.loadValue(t);
}

// saving string to BSON
template <class CharT, class Traits, class Alloc>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive& ar,
                                      std::basic_string<CharT, Traits, Alloc> const& str) {
    ar.saveValue(str);
}

// loading string from BSON
template <class CharT, class Traits, class Alloc>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar,
                                      std::basic_string<CharT, Traits, Alloc>& str) {
    ar.loadValue(str);
}

// ######################################################################
// Saving SizeTags to BSON
template <class T>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive&, cereal::SizeTag<T> const&) {
    // Nothing to do here, we don't explicitly save the size.
}

// Loading SizeTags from BSON
template <class T>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar, cereal::SizeTag<T>& st) {
    ar.loadSize(st.size);
}

// ######################################################################
// Saving BSON types to BSON
template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void CEREAL_SAVE_FUNCTION_NAME(BSONOutputArchive& ar, BsonT const& bsonVal) {
    ar.saveValue(bsonVal);
}

// Loading BSON types from BSON
template <class BsonT, cereal::traits::EnableIf<
                           is_bson<BsonT>::value ||
                           std::is_same<BsonT, std::chrono::system_clock::time_point>::value> =
                           cereal::traits::sfinae>
inline void CEREAL_LOAD_FUNCTION_NAME(BSONInputArchive& ar, BsonT& bsonVal) {
    ar.loadValue(bsonVal);
}

}  // namespace bson_mapper

// Register archives for polymorphic support.
CEREAL_REGISTER_ARCHIVE(bson_mapper::BSONInputArchive)
CEREAL_REGISTER_ARCHIVE(bson_mapper::BSONOutputArchive)

// Tie input and output archives together.
CEREAL_SETUP_ARCHIVE_TRAITS(bson_mapper::BSONInputArchive, bson_mapper::BSONOutputArchive)
