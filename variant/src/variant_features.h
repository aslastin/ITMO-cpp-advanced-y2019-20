#pragma once

#include <utility>
#include <array>
#include <type_traits>

template<class... Ts>
struct variant;

// in_place_index
template<size_t I>
struct in_place_index_t {
    explicit in_place_index_t() = default;
};

template<size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

// in_place_type
template<typename T>
struct in_place_type_t {
    explicit in_place_type_t() = default;
};

template<typename T>
inline constexpr in_place_type_t<T> in_place_type{};

// Type by index
template<size_t I, typename... Ts>
struct instance_by_index;

template<typename First, typename... Rest>
struct instance_by_index<0, First, Rest...> {
    using type = First;
};

template<size_t I, typename First, typename... Rest>
struct instance_by_index<I, First, Rest...> : instance_by_index<I - 1, Rest...> {
};

// Index by type
template<typename T, typename... Ts>
struct index_by_instance : std::integral_constant<size_t, 0> {
};

template<typename T, typename... Ts>
constexpr size_t index_by_instance_v = index_by_instance<T, Ts...>::value;

template<typename T, typename First, typename... Rest>
struct index_by_instance<T, First, Rest...>
        : std::integral_constant<size_t, std::is_same_v<T, First> ? 0 : index_by_instance_v<T, Rest...> + 1> {
};

// variant_size
template<typename Variant>
struct variant_size;

template<typename Variant>
struct variant_size<const Variant> : variant_size<Variant> {
};

template<typename Variant>
struct variant_size<volatile Variant> : variant_size<Variant> {
};

template<typename Variant>
struct variant_size<volatile const Variant> : variant_size<Variant> {
};

template<typename... Ts>
struct variant_size<variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {
};

template<typename Variant>
inline constexpr size_t variant_size_v = variant_size<Variant>::value;

// variant_alternative
template<size_t I, typename Variant>
struct variant_alternative;

template<size_t I, typename... Ts>
struct variant_alternative<I, variant<Ts...>> {
    using type = typename instance_by_index<I, Ts...>::type;
};

template<size_t I, typename Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

template<size_t I, typename Variant>
struct variant_alternative<I, const Variant> {
    using type = std::add_const_t<variant_alternative_t<I, Variant>>;
};

template<size_t I, typename Variant>
struct variant_alternative<I, volatile Variant> {
    using type = std::add_volatile_t<variant_alternative_t<I, Variant>>;
};

template<size_t I, typename Variant>
struct variant_alternative<I, volatile const Variant> {
    using type = std::add_cv_t<variant_alternative_t<I, Variant>>;
};

// variant_npos
constexpr size_t variant_npos = -1;

// Contains different constants for variant checks
template<typename... Ts>
struct Traits {
    static constexpr bool DEFAULT_CTOR = std::is_default_constructible_v<typename instance_by_index<0, Ts...>::type>;

    static constexpr bool COPY_CTOR = (std::is_copy_constructible_v<Ts> && ...);

    static constexpr bool MOVE_CTOR = (std::is_move_constructible_v<Ts> && ...);

    static constexpr bool COPY_ASSIGN = COPY_CTOR && (std::is_copy_assignable_v<Ts> && ...);

    static constexpr bool MOVE_ASSIGN = MOVE_CTOR && (std::is_move_assignable_v<Ts> && ...);

    static constexpr bool TRIVIAL_DTOR = (std::is_trivially_destructible_v<Ts> && ...);

    static constexpr bool TRIVIAL_COPY_CTOR = (std::is_trivially_copy_constructible_v<Ts> && ...);

    static constexpr bool TRIVIAL_MOVE_CTOR = (std::is_trivially_move_constructible_v<Ts> && ...);

    static constexpr bool TRIVIAL_COPY_ASSIGN = TRIVIAL_DTOR && TRIVIAL_COPY_CTOR &&
                                                (std::is_trivially_copy_assignable_v<Ts> && ...);

    static constexpr bool TRIVIAL_MOVE_ASSIGN = TRIVIAL_DTOR && TRIVIAL_MOVE_CTOR &&
                                                (std::is_trivially_move_assignable_v<Ts> && ...);

    static constexpr bool NOTHROW_DEFAULT_CTOR = std::is_nothrow_default_constructible_v<
            typename instance_by_index<0, Ts...>::type>;

    static constexpr bool NOTHROW_COPY_CTOR = (std::is_nothrow_copy_constructible_v<Ts> && ...);

    static constexpr bool NOTHROW_MOVE_CTOR = (std::is_nothrow_move_constructible_v<Ts> && ...);

    static constexpr bool NOTHROW_COPY_ASSIGN = NOTHROW_COPY_CTOR && (std::is_nothrow_copy_assignable_v<Ts> && ...);

    static constexpr bool NOTHROW_MOVE_ASSIGN = NOTHROW_MOVE_CTOR && (std::is_nothrow_move_assignable_v<Ts> && ...);
};

// Appropriate_index - helps converting constructor
template<typename T, typename Ti, size_t I, typename = void,
        bool = (!std::is_same_v<Ti, bool> || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, bool>)>
struct build_function {
    void f();
};

template<typename T, typename Ti, size_t I>
struct build_function<T, Ti, I, std::void_t<decltype(std::array<Ti, 1>{{std::declval<T>()}})>, true> {
    static std::integral_constant<size_t, I> f(Ti);
};

template<typename T, typename Variant,
        typename = std::make_index_sequence<variant_size_v<Variant>>>
struct build_functions;

template<typename T, typename... Ti, size_t... Is>
struct build_functions<T, variant<Ti...>, std::index_sequence<Is...>>
        : build_function<T, Ti, Is> ... {
    using build_function<T, Ti, Is>::f...;
};

template<typename T, typename Variant>
using appropriate_function_t = decltype(build_functions<T, Variant>::f(std::declval<T>()));

template<typename T, typename Variant, typename = void>
struct appropriate_index : std::integral_constant<size_t, variant_npos> {
};

template<typename T, typename Variant>
struct appropriate_index<T, Variant, std::void_t<appropriate_function_t<T, Variant>>>
        : appropriate_function_t<T, Variant> {
};

// is_unique_type
template<typename T, typename... Ts>
struct cnt_current;

template<typename T, typename... Ts>
constexpr size_t cnt_current_v = cnt_current<T, Ts...>::value;

template<typename T, typename First>
struct cnt_current<T, First> : std::integral_constant<size_t, std::is_same_v<T, First>> {
};

template<typename T, typename First, typename... Rest>
struct cnt_current<T, First, Rest...>
        : std::integral_constant<size_t, std::is_same_v<T, First> + cnt_current_v<T, Rest...>> {
};

template<typename T, typename... Ts>
constexpr bool is_unique_type = cnt_current_v<T, Ts...> == 1;
