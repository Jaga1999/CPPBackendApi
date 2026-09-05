#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>

namespace Domain::Common {

template <typename T>
concept Identifiable = requires(T entity) {
    { entity.id } -> std::integral;
};

template <typename T>
concept Validatable = requires(std::string_view text) {
    { T::validate(text) };
};

} // namespace Domain::Common
