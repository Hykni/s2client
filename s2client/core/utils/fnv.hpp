#pragma once

#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>

namespace fnv1a::detail
{
    template <std::size_t N>
    struct unsigned_by_size;
    template <>
    struct unsigned_by_size<8>
    {
        using type = uint64_t;
    };
    template <>
    struct unsigned_by_size<4>
    {
        using type = uint32_t;
    };
    template <>
    struct unsigned_by_size<2>
    {
        using type = uint16_t;
    };
    template <>
    struct unsigned_by_size<1>
    {
        using type = uint8_t;
    };
    template <std::size_t N>
    using unsigned_by_size_t = typename unsigned_by_size<N>::type;

    template <typename Type, Type OffsetBasis, Type Prime>
    struct SizeDependantData
    {
        using type = Type;

        constexpr static auto k_offset_basis = OffsetBasis;
        constexpr static auto k_prime = Prime;
    };

    template <std::size_t Bits>
    struct SizeSelector;

    template <>
    struct SizeSelector<32> : SizeDependantData<std::uint32_t, 0x811c9dc5ul, 16777619ul>
    {
    };

    template <>
    struct SizeSelector<64> : SizeDependantData<std::uint64_t, 0xcbf29ce484222325ull, 1099511628211ull>
    {
    };

    template <class T>
    struct is_character_type
    {
        constexpr static bool value = std::is_same<std::decay_t<T>, char>::value
            || std::is_same<std::decay_t<T>, wchar_t>::value
            || std::is_same<std::decay_t<T>, char16_t>::value
            || std::is_same<std::decay_t<T>, char32_t>::value;
    };

    template <std::size_t Size>
    class FnvHash
    {
    private:
        using data_t = SizeSelector<Size>;

    public:
        using hash = typename data_t::type;

        constexpr static auto k_offset_basis = data_t::k_offset_basis;
        constexpr static auto k_prime = data_t::k_prime;

    private:

        static __forceinline constexpr auto hash_single_internal(
            hash         current,
            std::uint8_t single) -> hash
        {
            return hash_byte(current, single);
        }

        template <class T>
        static __forceinline constexpr auto hash_single_internal(
            hash current,
            T    single) -> hash
        {
            constexpr std::size_t bytes = sizeof(T) == 1 ? 1 : sizeof(T) / 2;
            using half = unsigned_by_size_t<bytes>;

            auto hash_half = hash_single_internal(current, half((std::numeric_limits<half>::max)() & (single >> sizeof(T) / 2 * 8)));
            return hash_single_internal(hash_half, half((std::numeric_limits<half>::max)() & single));
        }

    public:
        static __forceinline constexpr auto hash_init() -> hash
        {
            return k_offset_basis;
        }

        static __forceinline constexpr auto hash_byte(
            hash         current,
            std::uint8_t byte) -> hash
        {
            return (current ^ byte) * k_prime;
        }

        template <typename T>
        static __forceinline constexpr auto hash_single(
            hash current,
            T    single) -> hash
        {
            return hash_single_internal(current, unsigned_by_size_t<sizeof(T)>(single));
        }

        template <typename T, std::size_t N>
        static __forceinline constexpr auto hash_constexpr(
            const T(&str)[N],
            // do not hash the null
            const std::size_t size = (is_character_type<T>::value ? N - 1 : N)) -> hash
        {
            auto cur_hash = hash_init();
            for (std::size_t i = 0; i < size; ++i)
                cur_hash = hash_single(cur_hash, str[i]);
            return cur_hash;
        }

        static auto __forceinline hash_runtime_data(
            const void* data,
            const std::size_t sz) -> hash
        {
            auto       begin = static_cast<const uint8_t*>(data);
            const auto end = begin + sz;
            auto       result = hash_init();
            for (; begin != end; ++begin)
                result = hash_byte(result, *begin);

            return result;
        }

        static auto __forceinline hash_wch_to_lower(
            const wchar_t* data,
            const std::size_t sz) -> hash
        {
            const auto end = data + sz;
            auto       result = hash_init();
            for (; data != end; ++data)
                result = hash_single(result, std::tolower(*data));

            return result;
        }

        static auto __forceinline hash_ch_to_lower(
            const char* data,
            const std::size_t sz) -> hash
        {
            const auto end = data + sz;
            auto       result = hash_init();
            for (; data != end; ++data)
                result = hash_single(result, std::tolower(*data));

            return result;
        }

        static auto __forceinline hash_rt_ch_to_lower(
            const char* str) -> hash
        {
            auto result = hash_init();
            while (*str)
                result = hash_single(result, std::tolower(*str++));

            return result;
        }

        static auto __forceinline hash_rt_wch_to_lower(
            const wchar_t* str) -> hash
        {
            auto result = hash_init();
            while (*str)
                result = hash_single(result,std::towlower(*str++));

            return result;
        }

        template <typename T>
        static auto __forceinline hash_runtime(
            const T* str) -> hash
        {
            auto result = hash_init();
            while (*str != T())
                result = hash_single(result, *str++);

            return result;
        }
    };
}

using fnv32 = ::fnv1a::detail::FnvHash<32>;
using fnv64 = ::fnv1a::detail::FnvHash<64>;
using fnv = ::fnv1a::detail::FnvHash<sizeof(void*) * 8>;

#define FNV(str) (std::integral_constant<fnv::hash, fnv::hash_constexpr(str)>::value)
#define FNV32(str) (std::integral_constant<fnv32::hash, fnv32::hash_constexpr(str)>::value)
#define FNV64(str) (std::integral_constant<fnv64::hash, fnv64::hash_constexpr(str)>::value)
