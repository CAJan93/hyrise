// #include "json_deserializer.hpp"
#include "json_serializer.hpp"

namespace opossum {
class JsonSerializerInterface {
 public:
  // serialize
  template <typename T>
  static auto from_json_str(const std::string& json_str);

  // serialize to string. Wrapper for to_json
  template <typename T>
  static std::string to_json_str(const T& object);
};

template <typename T>
std::string JsonSerializerInterface::to_json_str(const T& object) {
  return JsonSerializer::to_json(object).View().WriteReadable();
}

template <typename T>
auto JsonSerializerInterface::from_json_str(const std::string& json_str) {
  /*const jsonVal data(json_str);
  const jsonView jv = data.View();
  return from_json<T>(jv);*/ 
  // TODO(CAJan93)
  return -1;
}

}  // namespace opossum