#pragma once

#include <type_traits>

#include "variant_features.h"

// visitor implementation

namespace variant_helpers {
    template<size_t I, typename Variant>
    constexpr decltype(auto) get(Variant &&var) noexcept;
}

namespace visitor {
    // Own implementation because there is no constexpr in std::invoke
    template<typename F, typename... Args>
    constexpr decltype(auto) INVOKE(F &&f, Args &&... args) {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }

    // Indicates that visit was called inside variant
    struct internal_tag {
        using type = internal_tag;
    };

    // Indicates that visit was called inside variant + pass index as an argument
    struct internal_index_tag {
        using type = internal_index_tag;
    };

    template<typename T>
    constexpr bool is_additional_slot_required = std::is_same_v<internal_tag, T> ||
                                                 std::is_same_v<internal_index_tag, T>;

    // Here we are storing function pointers for visit
    template<typename Fptr, size_t... levels>
    struct Storage;

    // Multidimensional table
    template<typename Ret, typename Visitor, typename... Variants, size_t first, size_t... rest>
    struct Storage<Ret(*)(Visitor, Variants...), first, rest...> {
        using Fptr = Ret(*)(Visitor, Variants...);

        // Current level
        static constexpr size_t level = sizeof...(Variants) - sizeof...(rest) - 1;

        // When we are calling visit from variant, it might contain valuless value that's why
        // mustn't forget about creating slot for it too
        static constexpr int additional_slot = is_additional_slot_required<Ret>;

        template<typename... Is>
        constexpr decltype(auto) get(size_t index, Is... indices) const {
            return table[index + additional_slot].get(indices...);
        }

        Storage<Fptr, rest...> table[first + additional_slot];
    };

    // Cell of multidimensional table
    template<typename Fptr>
    struct Storage<Fptr> {
        template<typename T>
        struct parse_return;

        template<typename Res, typename... Args>
        struct parse_return<Res(*)(Args...)> {
            using Cell = Res(*)(Args...);
            using Return = Res;
        };

        template<typename... Args>
        struct parse_return<internal_tag(*)(Args...)> {
            using Cell = void (*)(Args...);
            using Return = void;
        };

        template<typename... Args>
        struct parse_return<internal_index_tag(*)(Args...)> {
            using Cell = void (*)(Args...);
            using Return = void;
        };

        using parsed_return = parse_return<Fptr>;

        constexpr const typename parsed_return::Cell &get() const {
            return cell;
        }

        typename parsed_return::Cell cell;
    };

    // Table is created inside this class
    template<typename Storage, typename Index_seq>
    struct Build_table;

    template<typename Ret, typename Visitor, typename... Variants, size_t... levels, size_t... indices>
    struct Build_table<Storage<Ret(*)(Visitor, Variants...), levels...>,
            std::index_sequence<indices...>> {
        using Current_storage = Storage<Ret(*)(Visitor, Variants...), levels...>;
        using Next = std::remove_reference_t<typename instance_by_index<sizeof...(indices), Variants...>::type>;

        // Initializing elements of current level
        static constexpr Current_storage init() {
            Current_storage storage{};
            init_alts(storage, std::make_index_sequence<variant_size_v<Next>>());
            return storage;
        }

        template<size_t... variant_indices>
        static constexpr void init_alts(Current_storage &storage, std::index_sequence<variant_indices...>) {
            if constexpr (is_additional_slot_required<Ret>) {
                init_solo_alt<variant_npos, true>(storage.table[0]);
                (init_solo_alt<variant_indices, false>(storage.table[variant_indices + 1]), ...);
            } else {
                (init_solo_alt<variant_indices, false>(storage.table[variant_indices]), ...);
            }
        }

        template<size_t index, bool is_additional, typename Storage_>
        static constexpr void init_solo_alt(Storage_ &storage) {
            if constexpr (is_additional) {
                storage = Build_table<std::remove_reference_t<decltype(storage)>,
                        std::index_sequence<indices..., index>>::init();
            } else {
                auto tmp = Build_table<std::remove_reference_t<decltype(storage)>,
                        std::index_sequence<indices..., index>>::init();
                static_assert(std::is_same_v<Storage_, decltype(tmp)>,
                              "visitor must have the same return type for all alternatives of the variant");
                storage = tmp;
            }
        }
    };

    // // Cell of multidimensional table
    template<typename Ret, typename Visitor, typename... Variants, size_t... indices>
    struct Build_table<Storage<Ret(*)(Visitor, Variants...)>,
            std::index_sequence<indices...>> {
        using Current_storage = Storage<Ret(*)(Visitor, Variants...)>;

        static constexpr auto init() {
            constexpr bool same_return = std::is_same_v<typename Current_storage::parsed_return::Return,
                    decltype(invoke_visitor(std::declval<Visitor>(), std::declval<Variants>()...))>;
            if constexpr (same_return) {
                return Current_storage{&invoke_visitor};
            } else {
                // Indicating that value is invalid
                return internal_tag{};
            }
        }

        static constexpr decltype(auto) invoke_visitor(Visitor &&visitor, Variants &&... vars) {
            if constexpr (std::is_same_v<Ret, internal_tag>) {
                INVOKE(std::forward<Visitor>(visitor),
                       alt_by_index<indices>(std::forward<Variants>(vars))...);
            } else if constexpr (std::is_same_v<Ret, internal_index_tag>) {
                INVOKE(std::forward<Visitor>(visitor),
                       alt_by_index<indices>(std::forward<Variants>(vars))...,
                       std::integral_constant<size_t, indices>()...);
            } else {
                return INVOKE(std::forward<Visitor>(visitor),
                              alt_by_index<indices>(std::forward<Variants>(vars))...);
            }
        }

        template<size_t index, typename Variant>
        static constexpr decltype(auto) alt_by_index(Variant &&var) noexcept {
            if constexpr (index != variant_npos) {
                return variant_helpers::get<index>(std::forward<Variant>(var));
            } else {
                // For failing function
                return internal_tag{};
            }
        }
    };

    template<typename Ret, typename Visitor, typename... Variants>
    struct Visit_table {
        using Table = Storage<Ret(*)(Visitor, Variants...), variant_size_v<std::remove_reference_t<Variants>>...>;
        static constexpr Table table = Build_table<Table, std::index_sequence<>>::init();
    };
}
