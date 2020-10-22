# Serializer

This is a short introduction to the hyrise PQP serializer. This serializer is needed to communicate with the worker nodes in [Skyrise](https://github.com/hpi-epic/skyrise). 

## Design principle: 

Information about the classes and files: The serializer consists of multiple classes. 

- **JsonSerializer** The serializer is responsible for serializing the PQP. 
- **JsonDeSerializer** The deerializer is responsible for serializing the PQP.  The DeSerializer and the Serializer both do not provide any public access. 
- **JsonSerializerInterface** This class provides a public interface via which you can (de)serailize a PQP. 
- **JsonSerializerUtil** Provides some utility functions used both in the JsonSerializer and the JsonDeserializer. 

Therer are other files which are used during (de)serialization. Among them are 

- **has_member** Uses SFINAE to determine if a class has provides a member. E.g. `if constexpr (has_member__type<T>::value)` will be true, if `T::_type` exists, no matter if `_type` is private or not. Read about it [here](https://stackoverflow.com/questions/64139547/how-to-detect-whether-there-is-a-specific-private-member-variable-in-class).
 
- **get_inner_type** Retrieves the inner type of a template. E.g. `get_inner_vec_t<std::vector<const int>::type` is `int`. 

## Basic Idea

The serializer is based on [this post](https://stackoverflow.com/a/34165367/5345715). The basic idea is as follows: Since there are no reflections in C++, we provide metainformation for each class. The (de)seralizer will then access this information and know which members of a class to (de)serialize and what type they are. The (de)serializer loops over all properties of a class using `JsonSerializerUtil::for_sequence`. 

During serialization member elements are set via `with_any`, which sets a key-value pair in the [AWS json](https://sdk.amazonaws.com/cpp/api/LATEST/namespace_aws_1_1_utils_1_1_json.html). Since the (de)serializer is aware to which what types a membervariable uses, we do not store type information. `std::make_shared<int> = 1`, `const int i = 1` and `int i = 1`will all have the value `1` in the json. 

The serializer achieves this by "drilling down" to the actual type. During serialization, "the outer" layer of a type `T` are removed step by step, which is why we only need to provide information about how `T` should be serialized and not how e.g. `std::vector<const std::shared_ptr<const T>>` should be serialized.  