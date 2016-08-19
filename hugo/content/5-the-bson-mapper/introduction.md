+++
next = "/5-the-bson-mapper/archives"
prev = "/5-the-bson-mapper/"
title = "Introduction"
toc = true
weight = 1

+++

For Mangrove to work, there must be a way to convert C++ objects into the BSON format that MongoDB uses to store documents on disk.

The traditional way of converting objects in a programming environment to an underlying data type is via {{% a_blank "serialization" "https://en.wikipedia.org/wiki/Serialization" %}}. There are many serialization libraries for C++, including {{% a_blank "boost::serialization" "http://www.boost.org/doc/libs/release/libs/serialization/" %}}, Google's {{% a_blank "Protocol Buffers" "https://developers.google.com/protocol-buffers/" %}}, and {{% a_blank "Cereal" "http://uscilab.github.io/cereal/" %}}.

For Mangrove, it was important that we went with a serialization library that was customizable (the BSON format is not currently as ubiquitous as XML, JSON, or Protocol Buffers), and that extensively utilized modern C++. {{% a_blank "Cereal" "http://uscilab.github.io/cereal/" %}} fit our requirements perfectly

At a high level, Cereal works via the concept of an **archive**. An archive lets a Cereal user take **serializable** objects and convert them into an underlying representation. An object can be made serializable by giving it a `serialize` function. Cereal comes packaged with archives for JSON, XML, and pure binary. An example of its usage is provided below:

```cpp

#include <fstream>
#include <cereal/archives/json.hpp>

class SomeData
{
public:
  uint8_t x, y;
  float z;
  
  // This function tells archives 
  // how to serialize this class
  template <class Archive>
  void serialize( Archive & ar )
  {
    ar( CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z) );
  }
};

int main() {
	std::ofstream os("out.json");
	cereal::JSONOutputArchive archive(os);

	// This creates a file called out.json
	// with the following contents:
	// {
	//     "myData": {
	//         "x": 1,
	//         "y": 2,
	//         "z": 3.1400001049041748
	//     }
	// }
	SomeData myData;
	myData.x = 1;
	myData.y = 2;
	myData.z = 3.14;
	archive(CEREAL_NVP(myData));
}

```

In the example above, we were able to create a JSON file out of the `SomeData` class with relative ease. Cereal also provides a `JSONInputArchive` to make it easy to **deserialize** JSON data back into C++ objects.

The **Boson** library that Mangrove uses internally provides a `BSONOutputArchive` and `BSONInputArchive` for Cereal that lets developers easily create BSON data out of C++ classes in a way that is idiomatic for Cereal users. Additionally, **Boson** provides several utility functions for mapping C++ objects to BSON without explicitly using Cereal archives.

**Boson** can be built and linked independently of Mangrove, so you can easily use it in your own projects if you'd like. The next two sections will discuss how to use the provided BSON archives and utility functions.