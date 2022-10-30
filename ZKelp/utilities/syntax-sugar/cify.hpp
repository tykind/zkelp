#ifndef GNR_CIFY_HPP
# define GNR_CIFY_HPP
# pragma once

#include <new>

#include <utility>

namespace gnr
{

    namespace cify_
    {

        template <typename>
        struct signature
        {
        };

        //
        template <typename>
        struct remove_cv_seq;

        //
        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...)>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) volatile>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const volatile>
        {
            using type = R(A...);
        };

        //
        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...)&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) volatile&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const volatile&>
        {
            using type = R(A...);
        };

        //
        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...)&&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const&&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) volatile&&>
        {
            using type = R(A...);
        };

        template <typename R, typename ...A>
        struct remove_cv_seq<R(A...) const volatile&&>
        {
            using type = R(A...);
        };

        template <typename F>
        constexpr inline auto extract_signature(F*) noexcept
        {
            return signature<typename remove_cv_seq<F>::type>();
        }

        template <typename C, typename F>
        constexpr inline auto extract_signature(F C::*) noexcept
        {
            return signature<typename remove_cv_seq<F>::type>();
        }

        template <typename F>
        constexpr inline auto extract_signature(F const&) noexcept ->
            decltype(&F::operator(), extract_signature(&F::operator()))
        {
            return extract_signature(&F::operator());
        }

        //////////////////////////////////////////////////////////////////////////////
        template <int I, typename F, typename R, typename ...A>
        inline auto cify(F&& fu, signature<R(A...)>) noexcept
        {
            static auto f(std::forward<F>(fu));

            if (static bool full; full)
            {
                f.~F();
                new (std::addressof(f)) F(std::forward<F>(fu));
            }
            else
            {
                full = true;
            }

            return +[](A... args) noexcept(noexcept(
                std::declval<F>()(std::forward<A>(args)...))) -> R
            {
                return f(std::forward<A>(args)...);
            };
        }

        template <int I, typename F, typename R, typename ...A>
        inline auto cify_once(F&& fu, signature<R(A...)>) noexcept
        {
            static auto f(std::forward<F>(fu));

            return +[](A... args) noexcept(noexcept(
                std::declval<F>()(std::forward<A>(args)...))) -> R
            {
                return f(std::forward<A>(args)...);
            };
        }

    }

    //////////////////////////////////////////////////////////////////////////////
    template <int I = 0, typename F>
    auto cify(F&& f) noexcept
    {
        return cify_::cify<I>(std::forward<F>(f),
            cify_::extract_signature(std::forward<F>(f))
            );
    }

    template <int I = 0, typename F>
    auto cify_once(F&& f) noexcept
    {
        return cify_::cify_once<I>(std::forward<F>(f),
            cify_::extract_signature(std::forward<F>(f))
            );
    }

}

#endif // GNR_CIFY_HPP
