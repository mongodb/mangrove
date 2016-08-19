+++
next = "/5-the-bson-mapper/utility-functions"
prev = "/5-the-bson-mapper/introduction"
title = "Using the BSON Archives"
toc = true
weight = 2

+++

**Boson** provides two new archives that can be used in the Cereal ecosystem: `BSONOutputArchive` for serializing C++ objects into BSON, and `BSONInputArchive` for deserializing BSON into C++ objects.

At a basic level, these archives work very similarly to the archives described in {{% a_blank "Cereal's documentation" "http://uscilab.github.io/cereal/serialization_archives.html" %}}, with a few minor quirks discussed on this page. 

If you haven't used Cereal before, we recommend reading its documentation so you have a basic familiarity with how it's used.

## Structure of Representation

The first important thing to understand about the BSON archives is how the underlying representation is structured.

We will refer to objects and values passed directly into a BSONOutputArchive as "root" objects and values. In the example below, a class called `MyData` and an int32_t called `someInt` are passed into the archive as a root object and a root value, respectively:

```cpp
class MyData
{
public:
    uint32_t x, y;
    float z;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};

int main() {
    std::ofstream os("out.bson");
    BSONOutputArchive archive(os);

    MyData a;
    int32_t i;

    /* set values of a and i */

    archive(a);
    archive(CEREAL_NVP(i));

    // This would also be equivalent:
    //     archive(a, CEREAL_NVP(i));

}
```

In the underlying BSON created by this archive, each "root" object or value is represented as a single BSON document (as defined in the {{% a_blank "BSON specification" "http://bsonspec.org/spec.html" %}}).

In the case of a value that isn't an object, the representation is a BSON document with a single element, where the element is that value.

The {{% a_blank "bsondump" "https://docs.mongodb.com/manual/reference/program/bsondump/" %}} output for `out.bson` in the above example would look something like this:

```json
{
    "x" : 1,
    "y" : 2,
    "z" : 3.14,
}
{
    "i" : 42
}
```

The `BSONInputArchive` has the same behavior. When populating a C++ object at the root level, it expects a BSON document with elements for each of its fields. When populating a value, it expects a document with a single element.

## Naming Requirements

Unlike other Cereal archives, the BSON archives require that most values are given a name (either using `CEREAL_NVP`, or `cereal::make_nvp`). The only values that don't need a name when passed into a BSON archive are C++ objects passed in at the root level.

Regular non-object values passed in at the root level still need a name because the the element in the single-element BSON document needs a name. C++ objects serialized as subdocuments also need to have a name, because subdocuments are normal named BSON elements.

When these naming requirements aren't met, the BSON archives will throw runtime exceptions with relevant error messages. 

## Out of Order Loading

Because every value in a BSON document has a name, the BSONInputArchive supports loading values for a particular document out of order. For example, if we had the following class:

```cpp
class MyData
{
public:
    uint32_t x, y;
    float z;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};
```

and the following two BSON documents:

```json
{
    "x" : 1,
    "y" : 2,
    "z" : 3.14
}
{
    "z" : 3.14,
    "x" : 1,
    "y" : 2
}
```

Both of the BSON documents above would be parsed in the same way.

{{% notice note %}}
Support for out of order loading is limited to within a single BSON document. If multiple objects or values are output by a BSONOutputArchive at the root level, they must be loaded in by a BSONInputArchive at the root level in the same order.
{{% /notice %}}

## Supported Types

The BSONOutputArchive and BSONInputArchive don't fully support every type that other Cereal archives support. Some containers don't currently have meaningful representations in BSON, and some types that may have a representation in JSON or XML don't have a proper BSON representation. In order to avoid undefined behavior, we only recommend using the types laid out in Chapter 2's [Allowed Types](/2-models/allowed-types) section.
