// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_UTIL_HPP)
#define RXCPP_RX_UTIL_HPP

#include "rx-includes.hpp"

#if !defined(RXCPP_THREAD_LOCAL)
#if defined(_MSC_VER)
#define RXCPP_THREAD_LOCAL __declspec(thread)
#else
#define RXCPP_THREAD_LOCAL __thread
#endif
#endif

#if !defined(RXCPP_SELECT_ANY)
#if defined(_MSC_VER)
#define RXCPP_SELECT_ANY __declspec(selectany)
#else
#define RXCPP_SELECT_ANY __attribute__((weak))
#endif
#endif

#define RXCPP_CONCAT(Prefix, Suffix) Prefix ## Suffix
#define RXCPP_CONCAT_EVALUATE(Prefix, Suffix) RXCPP_CONCAT(Prefix, Suffix)

#define RXCPP_MAKE_IDENTIFIER(Prefix) RXCPP_CONCAT_EVALUATE(Prefix, __LINE__)

namespace rxcpp {

namespace util {

template<class T, size_t size>
std::vector<T> to_vector(const T (&arr) [size]) {
    return std::vector<T>(std::begin(arr), std::end(arr));
}

template<class T0, class... TN>
typename std::enable_if<!std::is_array<T0>::value, std::vector<T0>>::type to_vector(T0 t0, TN... tn) {
    T0 arr[] = {t0, tn...};
    return to_vector(arr);
}

template<class T, T... ValueN>
struct values {};

template<class T, int Remaining, T Step = 1, T Cursor = 0, T... ValueN>
struct values_from;

template<class T, T Step, T Cursor, T... ValueN>
struct values_from<T, 0, Step, Cursor, ValueN...>
{
    typedef values<T, ValueN...> type;
};

template<class T, int Remaining, T Step, T Cursor, T... ValueN>
struct values_from
{
    typedef typename values_from<T, Remaining - 1, Step, Cursor + Step, ValueN..., Cursor>::type type;
};

template<bool... BN>
struct all_true;

template<bool B0>
struct all_true<B0>
{
    static const bool value = B0;
};
template<bool B0, bool... BN>
struct all_true<B0, BN...>
{
    static const bool value = B0 && all_true<BN...>::value;
};

namespace detail {

template<class F, class... ParamN, int... IndexN>
auto apply(std::tuple<ParamN...> p, values<int, IndexN...>, F& f)
    -> decltype(f(std::forward<ParamN>(std::get<IndexN>(p))...)) {
    return      f(std::forward<ParamN>(std::get<IndexN>(p))...);
}
template<class F, class... ParamN, int... IndexN>
auto apply(std::tuple<ParamN...> p, values<int, IndexN...>, const F& f)
    -> decltype(f(std::forward<ParamN>(std::get<IndexN>(p))...)) {
    return      f(std::forward<ParamN>(std::get<IndexN>(p))...);
}

}

template<class F, class... ParamN>
auto apply(std::tuple<ParamN...> p, F& f)
    -> decltype(detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f)) {
    return      detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f);
}
template<class F, class... ParamN>
auto apply(std::tuple<ParamN...> p, const F& f)
    -> decltype(detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f)) {
    return      detail::apply(std::move(p), typename values_from<int, sizeof...(ParamN)>::type(), f);
}

namespace detail {

template<class F>
struct apply_to
{
    F to;

    explicit apply_to(F f)
        : to(std::move(f))
    {
    }

    template<class... ParamN>
    auto operator()(std::tuple<ParamN...> p)
        -> decltype(rxcpp::util::apply(std::move(p), to)) {
        return      rxcpp::util::apply(std::move(p), to);
    }
    template<class... ParamN>
    auto operator()(std::tuple<ParamN...> p) const
        -> decltype(rxcpp::util::apply(std::move(p), to)) {
        return      rxcpp::util::apply(std::move(p), to);
    }
};

}

template<class F>
auto apply_to(F f)
    ->      detail::apply_to<F> {
    return  detail::apply_to<F>(std::move(f));
}

namespace detail {

struct pack
{
    template<class... ParamN>
    auto operator()(ParamN... pn)
        -> decltype(std::make_tuple(std::move(pn)...)) {
        return      std::make_tuple(std::move(pn)...);
    }
    template<class... ParamN>
    auto operator()(ParamN... pn) const
        -> decltype(std::make_tuple(std::move(pn)...)) {
        return      std::make_tuple(std::move(pn)...);
    }
};

}

inline auto pack()
    ->      detail::pack {
    return  detail::pack();
}

namespace detail {

template <class T>
class maybe
{
    bool is_set;
    typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type
        storage;
public:
    maybe()
    : is_set(false)
    {
    }

    maybe(T value)
    : is_set(false)
    {
        new (reinterpret_cast<T*>(&storage)) T(value);
        is_set = true;
    }

    maybe(const maybe& other)
    : is_set(false)
    {
        if (other.is_set) {
            new (reinterpret_cast<T*>(&storage)) T(other.get());
            is_set = true;
        }
    }
    maybe(maybe&& other)
    : is_set(false)
    {
        if (other.is_set) {
            new (reinterpret_cast<T*>(&storage)) T(std::move(other.get()));
            is_set = true;
            other.reset();
        }
    }

    ~maybe()
    {
        reset();
    }

    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    bool empty() const {
        return !is_set;
    }

    size_t size() const {
        return is_set ? 1 : 0;
    }

    iterator begin() {
        return reinterpret_cast<T*>(&storage);
    }
    const_iterator begin() const {
        return reinterpret_cast<T*>(&storage);
    }

    iterator end() {
        return reinterpret_cast<T*>(&storage) + size();
    }
    const_iterator end() const {
        return reinterpret_cast<T*>(&storage) + size();
    }

    T* operator->() {
        if (!is_set) abort();
        return reinterpret_cast<T*>(&storage);
    }
    const T* operator->() const {
        if (!is_set) abort();
        return reinterpret_cast<T*>(&storage);
    }

    T& operator*() {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }
    const T& operator*() const {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }

    T& get() {
        if (!is_set) abort();
        return *reinterpret_cast<T*>(&storage);
    }
    const T& get() const {
        if (!is_set) abort();
        return *reinterpret_cast<const T*>(&storage);
    }

    void reset()
    {
        if (is_set) {
            is_set = false;
            reinterpret_cast<T*>(&storage)->~T();
            //std::fill_n(reinterpret_cast<char*>(&storage), sizeof(T), 0);
        }
    }

    template<class U>
    void reset(U&& value) {
        reset();
        new (reinterpret_cast<T*>(&storage)) T(std::forward<U>(value));
        is_set = true;
    }

    maybe& operator=(const T& other) {
        reset(other);
        return *this;
    }
    maybe& operator=(const maybe& other) {
        if (const T* pother = other.get()) {
            reset(*pother);
        } else {
            reset();
        }
        return *this;
    }
};

}

namespace detail {
    struct surely
    {
        template<class... T>
        auto operator()(T... t)
            -> decltype(std::make_tuple(t.get()...)) {
            return      std::make_tuple(t.get()...);
        }
        template<class... T>
        auto operator()(T... t) const
            -> decltype(std::make_tuple(t.get()...)) {
            return      std::make_tuple(t.get()...);
        }
    };
}

template<class... T>
inline auto surely(const std::tuple<T...>& tpl)
    -> decltype(apply(tpl, detail::surely())) {
    return      apply(tpl, detail::surely());
}

namespace detail {

template<typename Function>
class unwinder
{
public:
    ~unwinder()
    {
        if (!!function)
        {
            try {
                (*function)();
            } catch (...) {
                std::unexpected();
            }
        }
    }

    explicit unwinder(Function* functionArg)
        : function(functionArg)
    {
    }

    void dismiss()
    {
        function = nullptr;
    }

private:
    unwinder();
    unwinder(const unwinder&);
    unwinder& operator=(const unwinder&);

    Function* function;
};

}

}
namespace rxu=util;

}

#define RXCPP_UNWIND(Name, Function) \
    RXCPP_UNWIND_EXPLICIT(uwfunc_ ## Name, Name, Function)

#define RXCPP_UNWIND_AUTO(Function) \
    RXCPP_UNWIND_EXPLICIT(RXCPP_MAKE_IDENTIFIER(uwfunc_), RXCPP_MAKE_IDENTIFIER(unwind_), Function)

#define RXCPP_UNWIND_EXPLICIT(FunctionName, UnwinderName, Function) \
    auto FunctionName = (Function); \
    rxcpp::util::detail::unwinder<decltype(FunctionName)> UnwinderName(std::addressof(FunctionName))

#endif
