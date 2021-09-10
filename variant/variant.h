#pragma once

#include <type_traits>
#include <cstddef>
#include <memory>
#include <variant>

#include "variant_features.h"
#include "variant_visitor.h"
#include "enable_special_members.h"

template <class... Ts>
struct variant;

template<typename Ret, typename Visitor, typename... Variants>
constexpr decltype(auto) apply_visit(Visitor&& visitor, Variants&&... vars);

namespace variant_helpers {
    /// variant_cast - кастует базу к variant с правильными квалификаторами
    template <typename... Ts, typename Variant_base>
    decltype(auto) variant_cast(Variant_base&& base) {
        if constexpr (std::is_lvalue_reference_v<Variant_base>) {
            if constexpr (std::is_const_v<std::remove_reference_t<Variant_base>>) {
                return static_cast<variant<Ts...> const&>(base);
            } else {
                return static_cast<variant<Ts...>&>(base);
            }
        } else {
            return static_cast<variant<Ts...>&&>(base);
        }
    }

    /// Геттеры
    template<typename Union>
    constexpr decltype(auto) get(in_place_index_t<0>, Union&& u) noexcept {
        return std::forward<Union>(u).first.get();
    }

    template<size_t I, typename Union>
    constexpr decltype(auto) get(in_place_index_t<I>, Union&& u) noexcept {
        return get(in_place_index<I - 1>, std::forward<Union>(u).rest);
    }

    template<size_t I, typename Variant>
    constexpr decltype(auto) get(Variant&& var) noexcept {
        return get(in_place_index<I>, std::forward<Variant>(var).u);
    }

    /// Вспомогательные функции для построения variant
    template <typename Left, typename Right_member>
    void construct_solo(Left&& left, Right_member&& right_member) {
        using T = std::remove_reference_t<Right_member>;
        if constexpr (!std::is_same_v<std::remove_reference_t<Right_member>, visitor::internal_tag>) {
            new (left.storage()) T(std::forward<Right_member>(right_member));
        }
    }

    template<typename... Ts, typename Left, typename Right>
    void construct(Left&& left, Right&& right) {
        apply_visit<visitor::internal_tag>(
                [&left](auto&& right_member) {
                    construct_solo(std::forward<Left>(left), std::forward<decltype(right_member)>(right_member));
                },
                variant_cast<Ts...>(std::forward<Right>(right)));
        left.current_index = right.current_index;
    }
}

namespace variant_implementation {
    /// Обертка над типом с геттерами для Union_storage. Всегда имеет тривиальный деструктор
    template <typename T, bool = std::is_trivially_destructible_v<T>>
    struct Storage {
        template <typename... Args>
        constexpr Storage(in_place_index_t<0>, Args&&... args)
                : data(std::forward<Args>(args)...)
        {}

        constexpr T& get() & noexcept {
            return data;
        }

        constexpr T const& get() const& noexcept {
            return data;
        }

        constexpr T&& get() && noexcept {
            return std::move(data);
        }

        constexpr T const&& get() const&& noexcept {
            return std::move(data);
        }

        T data;
    };

    template <typename T>
    struct Storage<T, false> {
        template <typename... Args>
        constexpr Storage(in_place_index_t<0>, Args&&... args) {
            new (&data) T(std::forward<Args>(args)...);
        }

        constexpr T& get() & noexcept {
            return *reinterpret_cast<T*>(&data);
        }

        constexpr T const& get() const& noexcept {
            return *reinterpret_cast<T const*>(&data);
        }

        constexpr T&& get() && noexcept {
            return std::move(*reinterpret_cast<T*>(&data));
        }

        constexpr T const&& get() const&& noexcept {
            return std::move(*reinterpret_cast<T const*>(&data));
        }

        typename std::aligned_storage<sizeof(T), alignof(T)>::type data;
    };

    /// Union_storage
    template <typename... Ts>
    union Union_storage
    {};

    template <typename First, typename... Rest>
    union Union_storage<First, Rest...> {
        constexpr Union_storage()
                : rest()
        {}

        template <typename... Args>
        constexpr Union_storage(in_place_index_t<0>, Args&&... args)
                : first(in_place_index<0>, std::forward<Args>(args)...)
        {}

        template <size_t I, typename... Args>
        constexpr Union_storage(in_place_index_t<I>, Args&&... args)
                : rest(in_place_index<I - 1>, std::forward<Args>(args)...)
        {}

        Storage<First> first;
        Union_storage<Rest...> rest;
    };

    /// Самая низкоуровневая база variant - trivial_dtor_base
    template <bool, typename... Ts>
    struct Trivial_dtor_base {
        constexpr Trivial_dtor_base()
                : current_index(variant_npos)
        {}

        template <size_t I, typename... Args>
        constexpr Trivial_dtor_base(in_place_index_t<I>, Args&& ... args)
                : u(in_place_index<I>, std::forward<Args>(args)...)
                , current_index{I}
        {}

        void reset() noexcept {
            current_index = variant_npos;
        }

        constexpr bool valid() const noexcept {
            return current_index != variant_npos;
        }

        void* storage() const noexcept {
            return const_cast<void*>(static_cast<const void*>(std::addressof(u)));
        }

        Union_storage<Ts...> u;
        size_t current_index;
    };

    template <typename... Ts>
    struct Trivial_dtor_base<false, Ts...> : Trivial_dtor_base<true, Ts...> {
        using Base = Trivial_dtor_base<true, Ts...>;
        using Base::Base;

        ~Trivial_dtor_base() {
            reset();
        }

        void reset() {
            if (Base::valid()) {
                apply_visit<void>(
                        [](auto&& value) {
                            std::destroy_at(std::addressof(value));
                        },
                        variant_helpers::variant_cast<Ts...>(*this));
                Base::reset();
            }
        }
    };

    template <typename... Ts>
    using Variant_trivial_dtor_base = Trivial_dtor_base<Traits<Ts...>::TRIVIAL_DTOR, Ts...>;

    template <bool, typename... Ts>
    struct Copy_ctor_base : Variant_trivial_dtor_base<Ts...> {
        using Base = Variant_trivial_dtor_base<Ts...>;
        using Base::Base;
    };

    template <typename... Ts>
    struct Copy_ctor_base<false, Ts...> : Variant_trivial_dtor_base<Ts...> {
        using Base = Variant_trivial_dtor_base<Ts...>;
        using Base::Base;

        constexpr Copy_ctor_base(Copy_ctor_base const& other) noexcept(Traits<Ts...>::NOTHROW_COPY_CTOR) {
            variant_helpers::construct<Ts...>(*this, other);
        }

        constexpr Copy_ctor_base(Copy_ctor_base&&) = default;
        constexpr Copy_ctor_base& operator=(Copy_ctor_base const&) = default;
        constexpr Copy_ctor_base& operator=(Copy_ctor_base&&) = default;
    };

    template <typename... Ts>
    using Variant_copy_ctor_base = Copy_ctor_base<Traits<Ts...>::TRIVIAL_COPY_CTOR, Ts...>;

    template <bool, typename... Ts>
    struct Move_ctor_base : Variant_copy_ctor_base<Ts...> {
        using Base = Variant_copy_ctor_base<Ts...>;
        using Base::Base;
    };

    template <typename... Ts>
    struct Move_ctor_base<false, Ts...> : Variant_copy_ctor_base<Ts...> {
        using Base = Variant_copy_ctor_base<Ts...>;
        using Base::Base;

        constexpr Move_ctor_base(Move_ctor_base const&) = default;

        constexpr Move_ctor_base(Move_ctor_base&& other) noexcept(Traits<Ts...>::NOTHROW_MOVE_CTOR) {
            variant_helpers::construct<Ts...>(*this, std::move(other));
        }

        constexpr Move_ctor_base& operator=(Move_ctor_base const&) = default;
        constexpr Move_ctor_base& operator=(Move_ctor_base&&) = default;
    };

    template <typename... Ts>
    using Variant_move_ctor_base = Move_ctor_base<Traits<Ts...>::TRIVIAL_MOVE_CTOR, Ts...>;

    template <bool, typename... Ts>
    struct Copy_assign_base : Variant_move_ctor_base<Ts...>{
        using Base = Variant_move_ctor_base<Ts...>;
        using Base::Base;
    };

    template <typename... Ts>
    struct Copy_assign_base<false, Ts...> : Variant_move_ctor_base<Ts...> {
        using Base = Variant_move_ctor_base<Ts...>;
        using Base::Base;

        constexpr Copy_assign_base(Copy_assign_base const&) = default;
        constexpr Copy_assign_base(Copy_assign_base&&) = default;

        constexpr Copy_assign_base& operator=(Copy_assign_base const& other) noexcept(Traits<Ts...>::NOTHROW_COPY_ASSIGN) {
            apply_visit<visitor::internal_index_tag>(
                    [this](auto&& other_member, auto other_index) {
                        if constexpr (other_index != variant_npos) {
                            if  (this->current_index != other_index) {
                                using other_t = std::remove_cv_t<std::remove_reference<decltype(other_member)>>;
                                if constexpr (std::is_nothrow_constructible_v<other_t > ||
                                              !std::is_nothrow_move_constructible_v<other_t>) {
                                    variant_helpers::variant_cast<Ts...>(*this).
                                            template emplace<other_index>(other_member);
                                } else {
                                    variant_helpers::variant_cast<Ts...>(*this) =
                                            variant<Ts...>(in_place_index<other_index>, other_member);
                                }
                            } else {
                                variant_helpers::get<other_index>(*this) = other_member;
                            }
                        } else {
                            this->reset();
                        }
                    }, variant_helpers::variant_cast<Ts...>(other));
            return *this;
        }

        constexpr Copy_assign_base& operator=(Copy_assign_base&&) = default;
    };

    template <typename... Ts>
    using Variant_copy_assign_base = Copy_assign_base<Traits<Ts...>::TRIVIAL_COPY_ASSIGN, Ts...>;

    template <bool, typename... Ts>
    struct Move_assign_base : Variant_copy_assign_base<Ts...> {
        using Base = Variant_copy_assign_base<Ts...>;
        using Base::Base;
    };

    template <typename... Ts>
    struct Move_assign_base<false, Ts...> : Variant_copy_assign_base<Ts...> {
        using Base = Variant_copy_assign_base<Ts...>;
        using Base::Base;

        constexpr Move_assign_base(Move_assign_base const&) = default;
        constexpr Move_assign_base(Move_assign_base&&) = default;
        constexpr Move_assign_base& operator=(Move_assign_base const&) = default;

        constexpr Move_assign_base& operator=(Move_assign_base&& other) noexcept(Traits<Ts...>::NOTHROW_MOVE_ASSIGN) {
            apply_visit<visitor::internal_index_tag>(
                    [this](auto&& other_member, auto other_index) {
                        if constexpr (other_index != variant_npos) {
                            if (this->current_index != other_index) {
                                variant_helpers::variant_cast<Ts...>(*this).
                                        template emplace<other_index>(std::move(other_member));
                            } else {
                                variant_helpers::get<other_index>(*this) = std::move(other_member);
                            }
                        } else {
                            this->reset();
                        }
                    }, variant_helpers::variant_cast<Ts...>(other));
            return *this;
        }
    };

    template <typename... Ts>
    using Variant_move_assign_base = Move_assign_base<Traits<Ts...>::TRIVIAL_MOVE_ASSIGN, Ts...>;

    template <typename... Ts>
    struct Variant_base : Variant_move_assign_base<Ts...> {
        using Base = Variant_move_assign_base<Ts...>;

        constexpr Variant_base() noexcept(Traits<Ts...>::NOTHROW_DEFAULT_CTOR)
                : Variant_base(in_place_index<0>)
        {}

        template <size_t I, typename... Args>
        constexpr Variant_base(in_place_index_t<I> index, Args&&... args)
                : Base(index, std::forward<Args>(args)...)
        {}

        constexpr Variant_base(Variant_base const&) = default;
        constexpr Variant_base(Variant_base&&) = default;
        constexpr Variant_base& operator=(Variant_base const&) = default;
        constexpr Variant_base& operator=(Variant_base&&) = default;
    };
}

struct bad_variant_access final : public std::exception {
    bad_variant_access() noexcept
    {}

    bad_variant_access(bad_variant_access const& other) noexcept {
        message = other.message;
    }

    char const* what() const noexcept override {
        return message;
    }

private:
    bad_variant_access(char const* message)
            : message(message)
    {}

    friend void throw_bad_variant_access(bool is_valueless);

    char const* message = "bad_variant_access";
};

void throw_bad_variant_access(bool is_valueless) {
    throw bad_variant_access(is_valueless ? "variant is valueless" : "get: wrong index for variant");
}

template <typename T, typename... Ts>
constexpr bool holds_alternative(variant<Ts...> const& var) noexcept {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return var.index() == index_by_instance_v<T, Ts...>;
}

template <class T, typename...  Ts>
constexpr T& get(variant<Ts...>& var) {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get<index_by_instance_v<T, Ts...>>(var);
}

template <class T, typename... Ts>
constexpr T&& get(variant<Ts...>&& var) {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get<index_by_instance_v<T, Ts...>>(std::move(var));
}

template <class T, typename... Ts>
constexpr T const& get(variant<Ts...> const& var) {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get<index_by_instance_v<T, Ts...>>(var);
}

template <class T, typename... Ts>
constexpr T const&& get(const variant<Ts...>&& var) {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get<index_by_instance_v<T, Ts...>>(std::move(var));
}

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...>& var) {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (I != var.index()) {
        throw_bad_variant_access(var.valueless_by_exception());
    }
    return variant_helpers::get<I>(var);
}

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...>&& var) {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (I != var.index()) {
        throw_bad_variant_access(var.valueless_by_exception());
    }
    return variant_helpers::get<I>(std::move(var));
}

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>> const& get(variant<Ts...> const& var) {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (I != var.index()) {
        throw_bad_variant_access(var.valueless_by_exception());
    }
    return variant_helpers::get<I>(var);
}

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>> const&& get(variant<Ts...> const&& var) {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (I != var.index()) {
        throw_bad_variant_access(var.valueless_by_exception());
    }
    return variant_helpers::get<I>(std::move(var));
}

template <size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>> get_if(variant<Ts...>* ptr) noexcept {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (ptr != nullptr && ptr->index() == I) {
        return std::addressof(variant_helpers::get<I>(*ptr));
    }
    return nullptr;
}

template <size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Ts...>> const> get_if(variant<Ts...> const* ptr) noexcept {
    static_assert(I < sizeof...(Ts), "index must be less than amount of alternatives");
    if (ptr != nullptr && ptr->index() == I) {
        return std::addressof(variant_helpers::get<I>(*ptr));
    }
    return nullptr;
}

template <class T, typename... Ts>
constexpr std::add_pointer_t<T> get_if(variant<Ts...>* ptr) noexcept {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get_if<index_by_instance_v<T, Ts...>>(ptr);
}

template <class T, typename... Ts>
constexpr std::add_pointer_t<T const> get_if(variant<Ts...> const* ptr) noexcept {
    static_assert(is_unique_type<T, Ts...>, "T must be unique alternative");
    return get_if<index_by_instance_v<T, Ts...>>(ptr);
}

template <typename... Ts>
constexpr bool operator==(variant<Ts...> const& left, variant<Ts...> const& right) {
    if (left.index() == right.index()) {
        return left.valueless_by_exception() || get<left.index()>(left) == std::get<left.index()>(right);
    }
    return false;
}

template <typename... Ts>
constexpr bool operator!=(variant<Ts...> const& left, variant<Ts...> const& right) {
    return !(left == right);
}

template <typename... Ts>
constexpr bool operator<(variant<Ts...> const& left, variant<Ts...> const& right) {
    if (left.valueless_by_exception() || left.index() < right.index()) {
        return true;
    }
    if (right.valueless_by_exception() || left.index() > right.index()) {
        return false;
    }
    return std::get<left.index()>(left) < std::get<left.index()>(right);
}

template <typename... Ts>
constexpr bool operator>(variant<Ts...> const& left, variant<Ts...> const& right) {
    return (left != right && !(left < right));
}

template <typename... Ts>
constexpr bool operator<=(variant<Ts...> const& left, variant<Ts...> const& right) {
    return !(left > right);
}

template <typename... Ts>
constexpr bool operator>=(variant<Ts...> const& left, variant<Ts...> const& right) {
    return !(left < right);
}

template <typename... Ts,
        typename = std::enable_if_t<(std::is_move_constructible_v<Ts> && ...) &&
                                    (std::is_swappable_v<Ts> && ...)>>
void swap(variant<Ts...>& left, variant<Ts...>& right) noexcept(noexcept(left.swap(right))) {
    left.swap(right);
}

template <typename... Ts>
struct variant : private variant_implementation::Variant_base<Ts...>,
                 private Enable_default_constructor<Traits<Ts...>::DEFAULT_CTOR>,
                 private Enable_copy_move<Traits<Ts...>::COPY_CTOR, Traits<Ts...>::COPY_ASSIGN,
                         Traits<Ts...>::MOVE_CTOR, Traits<Ts...>::MOVE_ASSIGN> {
    using Base = variant_implementation::Variant_base<Ts...>;
    using Default_ctor_base = Enable_default_constructor<Traits<Ts...>::DEFAULT_CTOR>;

    template <size_t I, typename = std::enable_if_t<I < sizeof...(Ts)>>
    using to_alternative = variant_alternative_t<I, variant>;

    template <typename T>
    static constexpr size_t allowed_index = appropriate_index<T, variant>::value;

    template <typename T>
    using allowed_type = to_alternative<allowed_index<T>>;

    template <typename T>
    static constexpr size_t index_by_type = index_by_instance_v<T, Ts...>;

    template <typename T>
    static constexpr bool is_unique = is_unique_type<T, Ts...>;

    static_assert(sizeof...(Ts) > 0, "variant must have at least one alternative");
    static_assert((std::is_destructible_v<Ts> && ...), "all alternatives must be destructible");
    static_assert(!(std::is_array_v<Ts> || ...), "array types types are not allowed in variant");
    static_assert((std::is_object_v<Ts> && ...), "non-object types are not allowed in variant");

    constexpr variant() = default;
    constexpr variant(variant const&) = default;
    constexpr variant(variant&&) = default;
    constexpr variant& operator=(variant const&) = default;
    constexpr variant& operator=(variant&&) = default;
    ~variant() = default;

    template <typename T,
            typename Tj = allowed_type<T>,
            typename = std::enable_if_t<is_unique<Tj> && std::is_constructible_v<Tj, T>>>
    constexpr variant(T&& obj) noexcept(std::is_nothrow_constructible_v<Tj, T>)
            : variant(in_place_index<allowed_index<T>>, std::forward<T>(obj))
    {}

    template <typename T, typename... Args,
            typename = std::enable_if_t<is_unique<T> && std::is_constructible_v<T, Args...>>>
    constexpr explicit variant(in_place_type_t<T>, Args&&... args)
            : variant(in_place_index<index_by_type<T>>, std::forward<Args>(args)...)
    {}

    template <size_t I, typename... Args,
            typename Alt = to_alternative<I>,
            typename = std::enable_if_t<std::is_constructible_v<Alt, Args...>>>
    constexpr explicit variant(in_place_index_t<I>, Args&&... args)
            : Base(in_place_index<I>, std::forward<Args>(args)...)
            , Default_ctor_base{Enable_default_constructor_tag{}}
    {}

    template <typename T,
            typename Tj = allowed_type<T>,
            typename = std::enable_if_t<is_unique<Tj> &&
                                        std::is_constructible_v<Tj, T> && std::is_assignable_v<Tj&, T>>>
    variant& operator=(T&& obj) noexcept(std::is_nothrow_constructible_v<Tj, T> &&
                                         std::is_nothrow_assignable_v<Tj&, T>) {
        constexpr auto pos = index_by_type<Tj>;
        if (pos == this->current_index) {
            get<pos>(*this) = std::forward<T>(obj);
        } else {
            if constexpr (std::is_nothrow_constructible_v<Tj, T> ||
                          !std::is_nothrow_move_constructible_v<Tj>) {
                this->emplace<pos>(std::forward<T>(obj));
            } else {
                this->operator=(variant(std::forward<T>(obj)));
            }
        }
        return *this;
    }

    constexpr size_t index() const noexcept {
        return this->current_index;
    }

    constexpr bool valueless_by_exception() const noexcept {
        return !this->valid();
    }

    template <class T, class... Args,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...> && is_unique<T>>>
    T& emplace(Args&&... args) {
        constexpr size_t i = index_by_type<T>;
        return this->emplace<i>(std::forward<Args>(args)...);
    }

    template <size_t I, class... Args,
            typename = std::enable_if_t<std::is_constructible_v<variant_alternative_t<I, variant>, Args...>>>
    variant_alternative_t<I, variant>& emplace(Args&&... args) {
        this->reset();
        this->current_index = variant_npos;
        new (this->storage()) std::remove_reference_t<decltype(get<I>(*this))> (std::forward<Args>(args)...);
        this->current_index = I;
        return get<I>(*this);
    }

    void swap(variant& right) noexcept(((std::is_nothrow_move_constructible_v<Ts> &&
                                         std::is_nothrow_swappable_v<Ts>) && ...)) {
        apply_visit<visitor::internal_index_tag>(
                [this, &right](auto&& right_member, auto right_index) {
                    if constexpr (right_index != variant_npos) {
                        if (this->index() == right_index) {
                            using std::swap;
                            swap(get<right_index>(*this), right_member);
                        } else {
                            if (!this->valueless_by_exception()) {
                                auto tmp(std::move(right_member));
                                right = std::move(*this);
                                this->reset();
                                variant_helpers::construct_solo(*this, std::move(tmp));
                                this->current_index = right_index;
                            } else {
                                this->reset();
                                variant_helpers::construct_solo(*this, std::move(right_member));
                                this->current_index = right_index;
                                right.reset();
                            }
                        }
                    } else {
                        if (!this->valueless_by_exception()) {
                            right = std::move(*this);
                            this->reset();
                        }
                    }
                }, right);
    }

private:

    template <typename... Types, typename Variant_base>
    friend decltype(auto) variant_helpers::variant_cast(Variant_base&& base);

    template<size_t I, typename Variant>
    friend constexpr decltype(auto) variant_helpers::get(Variant&& var) noexcept;

    template <typename Left, typename Right_member>
    friend void variant_helpers::construct_solo(Left&& left, Right_member&& right_member);

    template<typename... Types, typename Left, typename Right>
    friend void variant_helpers::construct(Left&& left, Right&& right);
};


template<typename Ret, typename Visitor, typename... Variants>
constexpr decltype(auto) apply_visit(Visitor&& visitor, Variants&&... vars) {
    constexpr auto& visit_table = visitor::Visit_table<Ret, Visitor&&, Variants&&...>::table;
    return (*visit_table.get(vars.index()...))(std::forward<Visitor>(visitor), std::forward<Variants>(vars)...);
}

template<typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... vars) {
    if ((vars.valueless_by_exception() || ...)) {
        throw_bad_variant_access(true);
    }
    using Ret = std::invoke_result_t<Visitor&&, decltype(get<0>(std::declval<Variants>()))...>;
    return apply_visit<Ret>(std::forward<Visitor>(visitor), std::forward<Variants>(vars)...);
}
