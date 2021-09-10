#pragma once

#include <type_traits>

struct nullopt_t {};
inline constexpr nullopt_t nullopt;

struct in_place_t {};
inline constexpr in_place_t in_place;

template <typename T, bool flag = std::is_trivially_destructible_v<T>>
struct opt_base {
    constexpr opt_base() noexcept
        : dummy()
        , has_value(false)
    {}

    constexpr opt_base(T value) noexcept
        : value(std::move(value))
        , has_value(true)
    {}

    template <typename... Args>
    explicit constexpr opt_base(in_place_t, Args&&... args)
        : value(std::forward<Args>(args)...)
        , has_value(true)
    {}

    ~opt_base() noexcept = default;

    void destroy() noexcept {}

 protected:
    union {
        char dummy;
        T value;
    };
    bool has_value;
};

template <typename T>
struct opt_base<T, false> {
    constexpr opt_base() noexcept
        : dummy(), has_value(false)
    {}

    constexpr opt_base(T value) noexcept
        : value(std::move(value))
        , has_value(true)
    {}

    template <typename... Args>
    explicit constexpr opt_base(in_place_t, Args&&... args)
        : value(std::forward<Args>(args)...)
        , has_value(true)
    {}

    ~opt_base() noexcept {
        destroy();
    }

    void destroy() noexcept {
        if (this->has_value) {
            this->value.~T();
        }
    }

 protected:
    union {
        char dummy;
        T value;
    };
    bool has_value;
};

template <typename T, bool flag = std::is_trivially_copyable_v<T>>
struct opt_base_c : opt_base<T> {
    using base = opt_base<T>;
    using base::base;
};

template <typename T>
struct opt_base_c<T, false> : opt_base<T> {
    using base = opt_base<T>;
    using base::base;

    opt_base_c(opt_base_c const& other) {
        if (other.has_value) {
            this->has_value = true;
            new (&this->value) T(other.value);
        } else {
            this->has_value = false;
        }
    }

    opt_base_c(opt_base_c&& other) noexcept
        : opt_base_c() {
        if (other.has_value) {
            new (&this->value) T(std::move(other.value));
            this->has_value = true;
        }
    }

    opt_base_c& operator=(opt_base_c const& other) {
        if (this != &other) {
            this->destroy();
            if (other.has_value) {
                new (&this->value) T(other.value);
                this->has_value = true;
            } else {
                this->has_value = false;
            }
        }
        return *this;
    }

    opt_base_c& operator=(opt_base_c&& other) noexcept {
        if (this != &other) {
            this->destroy();
            if (other.has_value) {
                new (&this->value) T(std::move(other.value));
                this->has_value = true;
            } else {
                this->has_value = false;
            }
        }
        return *this;
    }
};

template <typename T>
struct optional : opt_base_c<T> {
    using base = opt_base_c<T>;
    using base::base;

    constexpr optional(nullopt_t) noexcept
        : optional()
    {}

    optional& operator=(nullopt_t) noexcept {
        reset();
        return *this;
    }

    constexpr explicit operator bool() const noexcept {
        return this->has_value;
    }

    constexpr T& operator*() noexcept {
        return this->value;
    }

    constexpr T const& operator*() const noexcept {
        return this->value;
    }

    constexpr T* operator->() noexcept {
        return &this->value;
    }

    constexpr T const* operator->() const noexcept {
        return &this->value;
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        this->destroy();
        this->has_value = false;
        new (&this->value) T(std::forward<Args>(args)...);
        this->has_value = true;
    }

    void reset() noexcept {
        this->destroy();
        this->has_value = false;
    }

    template<typename A>
    friend constexpr bool operator==(optional<A> const &a, optional<A> const &b);
    template<typename A>
    friend constexpr bool operator!=(optional<A> const &a, optional<A> const &b);
    template<typename A>
    friend constexpr bool operator<(optional<A> const &a, optional<A> const &b);
    template<typename A>
    friend constexpr bool operator<=(optional<A> const &a, optional<A> const &b);
    template<typename A>
    friend constexpr bool operator>(optional<A> const &a, optional<A> const &b);
    template<typename A>
    friend constexpr bool operator>=(optional<A> const &a, optional<A> const &b);
};

template<typename T>
constexpr bool operator==(optional<T> const &a, optional<T> const &b) {
    return ((!a.has_value && !b.has_value) || (a.has_value && b.has_value && a.value == b.value));
}

template<typename T>
constexpr bool operator!=(optional<T> const &a, optional<T> const &b) {
    return !(a == b);
}

template<typename T>
constexpr bool operator<(optional<T> const &a, optional<T> const &b) {
    return ((!a.has_value && b.has_value) || (a.has_value && b.has_value && a.value < b.value));
}

template<typename T>
constexpr bool operator<=(optional<T> const &a, optional<T> const &b) {
    return (a < b || a == b);
}

template<typename T>
constexpr bool operator>(optional<T> const &a, optional<T> const &b) {
    return (a != b && !(a < b));
}

template<typename T>
constexpr bool operator>=(optional<T> const &a, optional<T> const &b) {
    return !(a < b);
}
