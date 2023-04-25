#pragma once

#include <array>
#include <map>
#include <set>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <memory>
#include <initializer_list>

#include "Exceptions.h"

using std::int32_t;
using std::int64_t;
using std::uint32_t;
using std::uint64_t;

constexpr float NEAR_ZERO = 1.0e-4f;

template <typename T>
inline float toFloat(T const& value)
{
    return static_cast<float>(value);
}

template <typename T>
inline double toDouble(T const& value)
{
    return static_cast<double>(value);
}

template<typename T>
inline int toInt(T const& value)
{
    return static_cast<int>(value);
}

#define THROW_NOT_IMPLEMENTED() throw std::runtime_error("not implemented")

#define CHECK(expression) \
    if (!(expression)) { \
        throw std::runtime_error("check failed"); \
    }

#define MEMBER_DECLARATION(className, type, name, initialValue) \
    type _##name = initialValue; \
    className& name(type const& name) \
    { \
        _##name = name; \
        return *this; \
    } \
    className& name(type&& name) \
    { \
        _##name = std::move(name); \
        return *this; \
    }

struct IntVector2D
{
    int x = 0;
    int y = 0;

    IntVector2D() = default;
    IntVector2D(std::initializer_list<int> l);
    bool operator==(IntVector2D const& vec) const;
    void operator-=(IntVector2D const& vec);
};

namespace std
{
    template <>
    struct hash<IntVector2D>
    {
        std::size_t operator()(IntVector2D const& value) const
        {
            using std::hash;
            using std::size_t;
            using std::string;

            size_t res = 17;
            res = res * 31 + hash<int>()(value.x);
            res = res * 31 + hash<int>()(value.y);
            return res;
        }
    };

}
struct RealVector2D
{
    float x = 0.0f;
    float y = 0.0f;

    RealVector2D() = default;
    RealVector2D(float x_, float y_);
    RealVector2D(std::initializer_list<float> l);
    auto operator<=>(RealVector2D const&) const = default;
    void operator+=(RealVector2D const& vec);
    void operator-=(RealVector2D const& vec);
    template <typename T>
    void operator*=(T factor)
    {
        x *= factor;
        y *= factor;
    }
    template <typename T>
    void operator/=(T divisor)
    {
        x /= divisor;
        y /= divisor;
    }
    template <typename T>
    RealVector2D operator*(T factor) const
    {
        return RealVector2D{x * factor, y * factor};
    }
    RealVector2D operator+(RealVector2D const& other) const;
    RealVector2D operator-(RealVector2D const& other) const;
    RealVector2D operator-() const;
    RealVector2D operator*(float factor) const;
    RealVector2D operator/(float divisor) const;
};

using RealMatrix2D = std::array<std::array<float, 2>, 2>;

inline IntVector2D toIntVector2D(RealVector2D const& v)
{
    return {static_cast<int>(v.x), static_cast<int>(v.y)};
}

struct RealRect
{
    RealVector2D topLeft;
    RealVector2D bottomRight;
};