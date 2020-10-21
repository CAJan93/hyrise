#pragma once

#include <memory>
#include <utility>

/**
 * Use these structs to check at compile time if an object of type is a smart pointer (in general or a specific one).
 *
 * Example:
 * if constexpr (is_unique_ptr<decltype(i)>::value) { ... // will execute, if i is a unique pointer
 * 
 * typename std::remove_cv_t<typename std::remove_reference_t<typename std::remove_cv_t<T>> is needed to process types like std::shared_ptr<AbstractOperator> const&
 *
 * Similar to is_pointer (see https://en.cppreference.com/w/cpp/types/is_pointer)
 */
namespace opossum {
// shared
namespace details {
template <class T>
struct is_shared_ptr_helper : std::false_type {};

template <class T>
struct is_shared_ptr_helper<std::shared_ptr<T>> : std::true_type {};
}  // namespace details

template <class T>
struct is_shared_ptr : details::is_shared_ptr_helper<std::remove_cv_t<std::remove_reference_t<std::remove_cv_t<T>>>> {};

// weak
namespace details {
template <class T>
struct is_weak_ptr_helper : std::false_type {};

template <class T>
struct is_weak_ptr_helper<std::weak_ptr<T>> : std::true_type {};
}  // namespace details

template <class T>
struct is_weak_ptr : details::is_weak_ptr_helper<std::remove_cv_t<std::remove_reference_t<std::remove_cv_t<T>>>> {};

// unique
namespace details {
template <class T>
struct is_unique_ptr_helper : std::false_type {};

template <class T>
struct is_unique_ptr_helper<std::unique_ptr<T>> : std::true_type {};
}  // namespace details

template <class T>
struct is_unique_ptr : details::is_unique_ptr_helper<std::remove_cv_t<std::remove_reference_t<std::remove_cv_t<T>>>> {};

// smart ptr
namespace details {
template <bool B, bool C, bool D>
struct is_smart_ptr_helper : std::true_type {};

template <>
struct is_smart_ptr_helper<false, false, false> : std::false_type {};
}  // namespace details

// retrus true if T is a smart pointer (shared, unique or weak). Ignores const, volatile and references
// e.g. is_smart_ptr<std::weak_ptr<SomeType> const&>::value is true
template <class T>
struct is_smart_ptr
    : details::is_smart_ptr_helper<is_unique_ptr<T>::value, is_shared_ptr<T>::value, is_weak_ptr<T>::value> {};
}  // namespace opossum