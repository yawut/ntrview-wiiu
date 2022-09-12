#pragma once

#include <functional>
#include <string>

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

class OnLeavingScope
{
public:
    // Use std::function so we can support
    // any function-like object
    using Func = std::function<void()>;

    // Prevent copying
    OnLeavingScope(const OnLeavingScope&) = delete;
    OnLeavingScope& operator=(const OnLeavingScope&) = delete;

    OnLeavingScope(const Func& f) :m_func(f) {}
   ~OnLeavingScope() { m_func(); }

private:
    Func m_func;
};

// Converts an **ASCII** std::string to UTF-16.
constexpr static std::u16string to_u16string(const std::string& s) {
    return {s.begin(), s.end()};
}

// Converts an **ASCII** wide string (std::u16string) to std::string.
constexpr static std::string to_string(const std::u16string& s) {
    return {s.begin(), s.end()};
}
