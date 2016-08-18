+++
next = "/4-updates/operators"
prev = "/5-the-bson-mapper/"
title = "Introduction"
toc = true
weight = 1

+++

For Mangrove to work, there must be a way to convert C++ objects into the BSON format that MongoDB uses to store documents on disk.

The traditional way of converting objects in a programming environment to an underlying data type is via {{% a_blank "serialization" "https://en.wikipedia.org/wiki/Serialization" %}}. There are many serialization libraries for C++, including {{% a_blank "boost::serialization" "http://www.boost.org/doc/libs/release/libs/serialization/" %}}, Google's {{% a_blank "Protocol Buffers" "https://developers.google.com/protocol-buffers/" %}}, and {{% a_blank "Cereal" "http://uscilab.github.io/cereal/" %}}.

For Mangrove, it was important that we went with a serialization library that was customizable (because BSON is not nearly as standard as XML or JSON), and that extensively utilized modern C++.