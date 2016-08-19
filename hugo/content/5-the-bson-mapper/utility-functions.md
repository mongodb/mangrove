+++
prev = "/5-the-bson-mapper/archives"
title = "Utility Functions"
toc = true
weight = 3

+++

In addition to the BSON archives, **Boson** provides several utility functions that make it easy to perform mappings between C++ objects and `bsoncxx` types. `bsoncxx` is the library used internally by the MongoDB C++ driver to represent BSON in memory.

### `to_document`

This function takes any Boson-serializable C++ object and turns it into a `bsoncxx::document::value`, which is an object containing BSON in memory. `bsoncxx::document::value` can be used with the MongoDB C++ driver, or its raw BSON can be accessed at the pointer provided by `.view().data()`.

```cpp
template <class T>
bsoncxx::document::value to_document(const T& obj);
```

#### `to_dotted_notation_document`

This function is similar to `to_document`, but the raw BSON it produces is in the {{% a_blank "dot notation" "https://docs.mongodb.com/manual/core/document/#dot-notation" %}} that can be used to update subdocuments with the MongoDB {{% a_blank "$set operator" "https://docs.mongodb.com/manual/reference/operator/update/set/" %}}.

```cpp
template <class T>
bsoncxx::document::value to_dotted_notation_document(const T& obj);
```

### `to_obj`

This function is essentially the inverse of `to_document`. It takes in a `bsoncxx::document::view` (which is a non-owning reference to a `bsoncxx::document::value`), and returns the Boson-serializable object that was represented by the raw BSON contained in the view.

```cpp
template <class T>
T to_obj(bsoncxx::document::view v);

template <class T>
void to_obj(bsoncxx::document::view v, T& obj);
```

#### `to_optional_obj`

In addition to the two signatures for `to_obj` shown above, an additional one is provided to support an optional `bsoncxx::document::view`. In this implementation, if the provided view doesn't exist, the function will simply return an empty optional.

```cpp
template <class T>
bsoncxx::stdx::optional<T> to_optional_obj(
    const bsoncxx::stdx::optional<bsoncxx::document::value>& opt);
```