#pragma once

namespace detail
{

    template<typename, typename, typename = void>
    constexpr bool handles_exception{};

    template<typename T, typename ExceptionType>
    constexpr bool handles_exception<
        T,
        ExceptionType,
        std::void_t<decltype(std::declval<T>().raise(std::declval<ExceptionType>()))>
    > = true;

    template<typename ObjectType, typename ReturnType, typename ArgType>
    constexpr ArgType catcher_arg(ReturnType(ObjectType::*)(ArgType));

    template<typename ObjectType, typename ReturnType, typename ArgType>
    constexpr ArgType catcher_arg(ReturnType(ObjectType::*)(ArgType) const);

    template<typename ExceptionType>
    struct Thrower
    {
        /*template<typename CatchingType>
        Thrower(CatchingType other, typename std::enable_if<handles_exception<CatchingType, ExceptionType>, int>::type p = 0)
        {
        }*/

        [[noreturn]] constexpr void raise(ExceptionType e) const
        {
            throw std::move(e);
        }

        [[noreturn]] constexpr void raise_nested(ExceptionType e) const
        {
            std::throw_with_nested(std::move(e));
        }
    protected:
        constexpr Thrower()
        { }

    };

    struct BypassExceptionCheckType {};
}

inline detail::BypassExceptionCheckType BypassExceptionCheck;

template<typename... ExceptionTypes>
struct Throws : public detail::Thrower<ExceptionTypes>...
{
    template<typename... OtherExceptionTypes>
    constexpr Throws(const Throws<OtherExceptionTypes...>& otherThrower)
    {
        static_assert((detail::handles_exception<Throws<OtherExceptionTypes...>, ExceptionTypes> && ...));
    }

    constexpr Throws(detail::BypassExceptionCheckType)
        : Throws()
    { }

    using detail::Thrower<ExceptionTypes>::raise...;
    using detail::Thrower<ExceptionTypes>::raise_nested...;

protected:
    constexpr Throws()
        : detail::Thrower<ExceptionTypes>()...
    { }
};

namespace detail
{
    template<class... Catchers>
    struct CatcherCollection : Catchers...
    {
        using Catchers::operator()...;

        template<typename BlockType>
        auto try_checked(const BlockType& block) const
        {
            using ThrowsType = Throws<decltype(detail::catcher_arg(&Catchers::operator()))...>;
            return try_checked_internal<BlockType, ThrowsType, Catchers...>(block);
        }

    private:
        template<typename BlockType, typename ThrowsType>
        auto try_checked_internal(const BlockType& block) const
        {
            return block(ThrowsType{ BypassExceptionCheck });
        }

        template<typename BlockType, typename ThrowsType, typename CatcherType, typename... MoreCatcherTypes>
        auto try_checked_internal(const BlockType& block) const
        {
            try
            {
                return try_checked_internal<BlockType, ThrowsType, MoreCatcherTypes...>(block);
            }
            catch (decltype(detail::catcher_arg(&CatcherType::operator())) e)
            {
                return CatcherType::operator()(e);
            }
        }
    };

    template<class... Ts> CatcherCollection(Ts...) -> CatcherCollection<Ts...>;

    template<typename CatcherType>
    struct SingleCatcher : public CatcherType
    {};

    struct SingleCatcherCreator
    {
        template<typename CatcherType>
        SingleCatcher<CatcherType> operator/(CatcherType&& catcher) &&
        {
            return SingleCatcher<CatcherType>{std::forward<CatcherType>(catcher)};
        }
    };

    template<typename BlockType, typename... Catchers>
    struct CheckCombiner
    {
        const BlockType& block;
        CatcherCollection<Catchers...> catchers;

        template<typename NewCatcherType>
        CheckCombiner<BlockType, NewCatcherType, Catchers...> operator +(SingleCatcher<NewCatcherType>&& new_catcher) &&
        {
            return {block, std::forward<NewCatcherType>(new_catcher), std::forward<Catchers>(catchers)...};
        }
    };

    struct CheckCombinerCreator
    {
        template<typename BlockType>
        CheckCombiner<BlockType> operator/(BlockType&& block) &&
        {
            return { std::forward<BlockType>(block) };
        }
    };

    struct CheckedExecutor
    {
        template<typename BlockType, typename... Catchers>
        auto operator=(CheckCombiner<BlockType, Catchers...>&& combiner) &&
        {
            return combiner.catchers.try_checked(combiner.block);
        }
    };
}

#define try_checked detail::CheckedExecutor() = detail::CheckCombinerCreator() / [&](const auto& except)
#define catch_checked(signature) + detail::SingleCatcherCreator() / [&](signature)

