#pragma once

struct Enable_default_constructor_tag {
};

template<bool Switch>
struct Enable_default_constructor {
    constexpr Enable_default_constructor() noexcept = default;

    constexpr Enable_default_constructor(Enable_default_constructor const &) noexcept = default;

    constexpr Enable_default_constructor(Enable_default_constructor &&) noexcept = default;

    Enable_default_constructor &operator=(Enable_default_constructor const &) noexcept = default;

    Enable_default_constructor &operator=(Enable_default_constructor &&) noexcept = default;

    constexpr explicit Enable_default_constructor(Enable_default_constructor_tag) {}
};

template<>
struct Enable_default_constructor<false> {
    constexpr Enable_default_constructor() noexcept = delete;

    constexpr Enable_default_constructor(Enable_default_constructor const &) noexcept = default;

    constexpr Enable_default_constructor(Enable_default_constructor &&) noexcept = default;

    Enable_default_constructor &operator=(Enable_default_constructor const &) noexcept = default;

    Enable_default_constructor &operator=(Enable_default_constructor &&) noexcept = default;

    constexpr explicit Enable_default_constructor(Enable_default_constructor_tag) {}
};

template<bool Switch>
struct Enable_copy_ctor {
    constexpr Enable_copy_ctor() noexcept = default;

    constexpr Enable_copy_ctor(Enable_copy_ctor const &) noexcept = default;

    constexpr Enable_copy_ctor(Enable_copy_ctor &&) noexcept = default;

    constexpr Enable_copy_ctor &operator=(Enable_copy_ctor const &) noexcept = default;

    constexpr Enable_copy_ctor &operator=(Enable_copy_ctor &&) noexcept = default;
};

template<>
struct Enable_copy_ctor<false> {
    constexpr Enable_copy_ctor() noexcept = default;

    constexpr Enable_copy_ctor(Enable_copy_ctor const &) noexcept = delete;

    constexpr Enable_copy_ctor(Enable_copy_ctor &&) noexcept = default;

    constexpr Enable_copy_ctor &operator=(Enable_copy_ctor const &) noexcept = default;

    constexpr Enable_copy_ctor &operator=(Enable_copy_ctor &&) noexcept = default;
};

template<bool Switch>
struct Enable_copy_assign {
    constexpr Enable_copy_assign() noexcept = default;

    constexpr Enable_copy_assign(Enable_copy_assign const &) noexcept = default;

    constexpr Enable_copy_assign(Enable_copy_assign &&) noexcept = default;

    constexpr Enable_copy_assign &operator=(Enable_copy_assign const &) noexcept = default;

    constexpr Enable_copy_assign &operator=(Enable_copy_assign &&) noexcept = default;
};

template<>
struct Enable_copy_assign<false> {
    constexpr Enable_copy_assign() noexcept = default;

    constexpr Enable_copy_assign(Enable_copy_assign const &) noexcept = default;

    constexpr Enable_copy_assign(Enable_copy_assign &&) noexcept = default;

    constexpr Enable_copy_assign &operator=(Enable_copy_assign const &) noexcept = delete;

    constexpr Enable_copy_assign &operator=(Enable_copy_assign &&) noexcept = default;
};


template<bool Switch>
struct Enable_move_ctor {
    constexpr Enable_move_ctor() noexcept = default;

    constexpr Enable_move_ctor(Enable_move_ctor const &) noexcept = default;

    constexpr Enable_move_ctor(Enable_move_ctor &&) noexcept = default;

    constexpr Enable_move_ctor &operator=(Enable_move_ctor const &) noexcept = default;

    constexpr Enable_move_ctor &operator=(Enable_move_ctor &&) noexcept = default;
};

template<>
struct Enable_move_ctor<false> {
    constexpr Enable_move_ctor() noexcept = default;

    constexpr Enable_move_ctor(Enable_move_ctor const &) noexcept = default;

    constexpr Enable_move_ctor(Enable_move_ctor &&) noexcept = delete;

    constexpr Enable_move_ctor &operator=(Enable_move_ctor const &) noexcept = default;

    constexpr Enable_move_ctor &operator=(Enable_move_ctor &&) noexcept = default;
};

template<bool Switch>
struct Enable_move_assign {
    constexpr Enable_move_assign() noexcept = default;

    constexpr Enable_move_assign(Enable_move_assign const &) noexcept = default;

    constexpr Enable_move_assign(Enable_move_assign &&) noexcept = default;

    constexpr Enable_move_assign &operator=(Enable_move_assign const &) noexcept = default;

    constexpr Enable_move_assign &operator=(Enable_move_assign &&) noexcept = default;
};

template<>
struct Enable_move_assign<false> {
    constexpr Enable_move_assign() noexcept = default;

    constexpr Enable_move_assign(Enable_move_assign const &) noexcept = default;

    constexpr Enable_move_assign(Enable_move_assign &&) noexcept = default;

    constexpr Enable_move_assign &operator=(Enable_move_assign const &) noexcept = default;

    constexpr Enable_move_assign &operator=(Enable_move_assign &&) noexcept = delete;
};

template<bool Copy, bool Copy_assignment, bool Move, bool Move_assignment>
struct Enable_copy_move : Enable_copy_ctor<Copy>, Enable_copy_assign<Copy_assignment>,
                          Enable_move_ctor<Move>, Enable_move_assign<Move_assignment> {
};
