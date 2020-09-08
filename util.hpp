#pragma once

#include <functional>

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
