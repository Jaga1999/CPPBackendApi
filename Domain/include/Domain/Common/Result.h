#pragma once

#include "Domain/Common/ValidationError.h"
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Domain::Common {

template <typename T, typename E = DomainError>
class Result {
public:
    static Result ok(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }

    static Result err(E error) {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] bool isOk() const noexcept {
        return m_storage.index() == 0;
    }

    [[nodiscard]] bool isErr() const noexcept {
        return m_storage.index() == 1;
    }

    [[nodiscard]] bool isSuccess() const noexcept {
        return isOk();
    }

    [[nodiscard]] bool isFailure() const noexcept {
        return isErr();
    }

    [[nodiscard]] const T& value() const & {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value on Error Result");
        }
        return std::get<0>(m_storage);
    }

    [[nodiscard]] T&& value() && {
        if (isErr()) {
            throw std::runtime_error("Attempted to access value on Error Result");
        }
        return std::get<0>(std::move(m_storage));
    }

    [[nodiscard]] const E& error() const & {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error on Ok Result");
        }
        return std::get<1>(m_storage);
    }

    [[nodiscard]] E&& error() && {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error on Ok Result");
        }
        return std::get<1>(std::move(m_storage));
    }

private:
    template <std::size_t I, typename... Args>
    explicit Result(std::in_place_index_t<I> tag, Args&&... args)
        : m_storage(tag, std::forward<Args>(args)...) {}

    std::variant<T, E> m_storage;
};

template <typename E>
class Result<void, E> {
public:
    static Result ok() {
        return Result();
    }

    static Result err(E error) {
        Result r;
        r.m_error = std::move(error);
        return r;
    }

    [[nodiscard]] bool isOk() const noexcept {
        return !m_error.has_value();
    }

    [[nodiscard]] bool isErr() const noexcept {
        return m_error.has_value();
    }

    [[nodiscard]] bool isSuccess() const noexcept {
        return isOk();
    }

    [[nodiscard]] bool isFailure() const noexcept {
        return isErr();
    }

    [[nodiscard]] const E& error() const & {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error on Ok Result");
        }
        return *m_error;
    }

    [[nodiscard]] E&& error() && {
        if (isOk()) {
            throw std::runtime_error("Attempted to access error on Ok Result");
        }
        return std::move(*m_error);
    }

private:
    std::optional<E> m_error{std::nullopt};
};

} // namespace Domain::Common
