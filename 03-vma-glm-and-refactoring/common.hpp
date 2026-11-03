// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#pragma once

#include <span>
#include <cstdio>
#include <memory>
#include <string>
#include <cstddef>
#include <variant>
#include <cassert>
#include <type_traits>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct Ok {};

// A very simple Result type to hold either
// an error message or the value. To be used
// mostly as return types in failable functions. 
template<typename T = Ok>
class [[nodiscard]] Result
{
    static_assert(!std::is_same_v<std::decay_t<T>, std::string>, "std::string values are not allowed");
    static_assert(!std::is_same_v<T, void>, "void values are not allowed. Use Result<> or Result<Ok> instead.");

public:
    Result(std::nullptr_t) = delete;

    constexpr Result(const char* error): m_variant(std::in_place_index<1>, error) {}
    constexpr Result(T&& value): m_variant(std::in_place_index<0>, std::move(value)) {}
    constexpr Result(const std::string& error): m_variant(std::in_place_index<1>, error) {}

    constexpr bool is_ok() const { return m_variant.index() == 0; }
    constexpr bool is_error() const { return m_variant.index() == 1; }

    T get_value() && = delete;
    T& get_value() & { assert(is_ok()); return std::get<0>(m_variant); }
    const T& get_value() const& { assert(is_ok()); return std::get<0>(m_variant); }

    T take_value() && { assert(is_ok()); return std::get<0>(std::move(m_variant)); }
    
    const std::string& get_error() const& { assert(is_error()); return std::get<1>(m_variant); }
    std::string get_error() && { assert(is_error()); return std::get<1>(std::move(m_variant)); }

private:
    std::variant<T, std::string> m_variant;
};

// Alias std::unique_ptr to Box. Easier to type.
// (Inspired by Rust of course, just like Result above).
template<typename T>
using Box = std::unique_ptr<T>;
