#ifndef XML_HPP_INCLUDED
#define XML_HPP_INCLUDED

#include "util/string.hpp"

#include <optional>

namespace vb::xml {

using namespace std::literals;

enum class TagType { EMPTY, START, END };

using AttributeValue = std::optional<std::string_view>;
using AttributeName  = std::string_view;

template <std::size_t N>
using AttrNames = std::array<AttributeName, N>;

template <std::size_t N>
using AttrValues = std::array<AttributeValue, N>;

template <typename XML_NODE_CONTENT>
concept node_type =
    requires(const XML_NODE_CONTENT& node) {
	    { XML_NODE_CONTENT::tag_name } -> std::convertible_to<std::string_view>;
	    { XML_NODE_CONTENT::attribute_names } -> std::ranges::forward_range;
	    { *std::ranges::begin(XML_NODE_CONTENT::attribute_names) } -> std::convertible_to<std::string_view>;
	    { node.template get<std::string_view> } -> std::convertible_to<std::string_view>;
    };

template <typename NODE_CONTENT>
concept single_node_content =
    std::convertible_to<NODE_CONTENT, std::string_view> ||
    node_type<NODE_CONTENT>;

template <typename NODE_CONTENT>
concept node_content =
    single_node_content<NODE_CONTENT> ||
    (std::ranges::range<NODE_CONTENT> &&
     single_node_content<std::ranges::range_value_t<NODE_CONTENT>>);

template <typename XML_NODE_CONTENT>
concept populated_node =
    node_type<XML_NODE_CONTENT> && requires(const XML_NODE_CONTENT& node) {
	                                   { node.content() } -> node_content;
                                   };

template <TagType TYPE, node_type NODE_TYPE>
constexpr auto tag(NODE_TYPE node) {
	std::string result{"<"};

	if constexpr (TYPE == TagType::END) {
		result += '/';
	}

	result += std::string_view(NODE_TYPE::tag_name);

	if constexpr (TYPE != TagType::END) {
		for (const auto attr : NODE_TYPE::attribute_names) {
			result += " "s + attr + "='"s + node.template get<attr>() + "'"s;
		}
	}

	if constexpr (TYPE == TagType::EMPTY) {
		result += '/';
	}

	return result + ">";
}

template <node_type TYPE>
constexpr auto build(TYPE&& node) {
	if constexpr (std::convertible_to<TYPE, std::string_view>) {
		return std::string_view(std::forward<TYPE>(node));
	} else if constexpr (populated_node<TYPE>) {
		return tag<TagType::START>(node) + build(node.content()) +
		       tag<TagType::END>(node);
	} else {
		return tag<TagType::EMPTY>(node);
	}
}

template <static_string NAME, static_string... ATTR_NAMES>
struct node {
	static constexpr auto name = std::string_view(NAME);
	static constexpr auto attribute_names =
	    AttrNames<sizeof...(ATTR_NAMES)>{ATTR_NAMES...};
	static constexpr auto NO_ATTRIBUTE = sizeof...(ATTR_NAMES);

	template <static_string ATTR>
	    requires((ATTR == ATTR_NAMES) || ...)
	static constexpr auto index = []() {
		return std::distance(std::begin(attribute_names),
		                     std::ranges::find(attribute_names, ATTR));
	}();

	template <static_string ATTR>
	    requires((ATTR == ATTR_NAMES) || ...)
	inline auto get() const {
		return attribute_values[index<ATTR>];
	}

	AttrValues<sizeof...(ATTR_NAMES)> attribute_values;

	constexpr auto xml() {
		return tag<TagType::EMPTY>(name, attribute_names, attribute_values);
	}
};

}

#endif
