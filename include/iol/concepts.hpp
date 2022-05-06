#ifndef IOL_CONCEPTS_HPP
#define IOL_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

namespace iol
{

template <typename T>
concept movable_type =
    std::move_constructible<std::decay_t<T>> && std::constructible_from<std::decay_t<T>, T>;

template <typename From, typename To>
concept decays_to = std::same_as<std::decay_t<From>, To>;

template <typename T>
concept class_type = decays_to<T, T> && std::is_class_v<T>;

}  // namespace iol

#endif  // IOL_CONCEPTS_HPP
