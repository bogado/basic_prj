#ifndef ARRAYS_HPP_INCLUDED
#define ARRAYS_HPP_INCLUDED

#include <array>
#include <span>

namespace vb {

template <typename T>
concept is_span = std::same_as<T, std::span<typename T::value_type, T::extent>>;

static_assert(is_span<std::span<char>>);
static_assert(is_span<std::span<char,10>>);
static_assert(!is_span<std::array<char, 3>>);

template <typename T>
concept is_dynamic_span = is_span<T> && T::extent == std::dynamic_extent;

static_assert(is_dynamic_span<std::span<char>>);
static_assert(!is_dynamic_span<std::span<char,10>>);
static_assert(!is_dynamic_span<std::array<char, 3>>);

template <typename T>
concept is_static_span = is_span<T> && T::extent != std::dynamic_extent;

static_assert(!is_static_span<std::span<char>>);
static_assert(is_static_span<std::span<char,10>>);
static_assert(!is_static_span<std::array<char, 3>>);

template <typename T>
concept is_std_array = requires(T t) {
    { std::array<typename T::value_type, std::tuple_size_v<T>>{} } -> std::same_as<T>;
};

static_assert(is_std_array<std::array<char, 3>>);
static_assert(is_std_array<std::array<char, 0>>);

template <typename T>
concept constexpr_sized =
    std::is_bounded_array_v<std::remove_cvref_t<T>>
    || is_static_span<T>
    || is_std_array<T>;

static_assert(constexpr_sized<std::array<char, 3>>);
static_assert(constexpr_sized<std::span<char, 3>>);
static_assert(!constexpr_sized<std::span<char>>);
static_assert(constexpr_sized<decltype("asd")>);

template <constexpr_sized T>
constexpr auto constexpr_size_of = []() {
    using TYPE = std::remove_cvref_t<T>;
    if constexpr (std::is_bounded_array_v<TYPE>)
        return std::extent_v<TYPE>;
    else if constexpr ( requires { requires std::integral<decltype(TYPE::extent)>; } )
        return TYPE::extent;
    else if constexpr (requires { std::tuple_size_v<TYPE>; })
        return std::tuple_size_v<TYPE>;
    else
       throw "Not constexpr sized";
}();

template <template <typename, std::integral auto> class FIXED_CONTAINER>
    concept is_fixed_container = constexpr_sized<FIXED_CONTAINER<char, 1>> == 1;

template <constexpr_sized ARRAY_LIKE_T>
constexpr auto to_array(const ARRAY_LIKE_T array_like) {
	auto result = std::array<
        std::ranges::range_value_t<ARRAY_LIKE_T>,
        constexpr_size_of<ARRAY_LIKE_T>>{};

	std::ranges::copy(array_like, result.begin());

	return result;
}

template <typename T, typename... OTHERS>
requires(! constexpr_sized<T> && (std::is_convertible_v<OTHERS, T> && ...))
constexpr auto array_of(const T value, const OTHERS... others)
{
    return std::array{value, static_cast<T>(others)...};
}

static_assert(std::ranges::equal(array_of(1,2,3), std::array {1,2,3}));
static_assert(std::ranges::equal(array_of('a', 'b', 0), std::array{'a', 'b', '\0'}));

template <constexpr_sized FIRST_T, constexpr_sized ... COLLECTION_Ts>
requires (std::convertible_to<typename std::ranges::range_value_t<COLLECTION_Ts>, std::ranges::range_value_t<FIRST_T>> && ...)
consteval auto merge_arrays(const FIRST_T& first, const COLLECTION_Ts& ... collections) {
	using value_type = std::ranges::range_value_t<FIRST_T>;

	constexpr auto first_size = constexpr_size_of<FIRST_T>;
	constexpr auto size = first_size + (constexpr_size_of<COLLECTION_Ts> + ...);
	auto           result = std::array<value_type, size>{};

	auto iterator = std::begin(result);
    if constexpr (first_size > 0) {
	    iterator = std::ranges::copy(first, iterator).out;
    }

	((iterator = std::ranges::copy(collections, iterator).out), ...);

	return result;
}

static_assert(std::ranges::equal(merge_arrays(std::array<char, 1>{'a'}, std::array{'a', 'b', 'c'}), std::string_view("aabc")));
static_assert(std::ranges::equal(merge_arrays(std::array<char, 0>{}, "abc"), std::array{'a', 'b', 'c', '\0'}));
static_assert(merge_arrays(std::array<char, 0>{}, std::array<char, 1>{'a'})[0] == 'a');

}
#endif
