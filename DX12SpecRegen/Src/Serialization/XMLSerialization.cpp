#include "XMLSerialization.h"

#include <spdlog/fmt/fmt.h>

namespace Serialization
{
	static std::string XMLEncode(std::string_view str)
	{
		std::string copy;
		copy.reserve(str.size());
		std::size_t offset = 0;
		for (std::size_t i = 0; i < str.size(); ++i)
		{
			char c = str[i];
			switch (c)
			{
			case '"':
				copy += str.substr(offset, i - offset);
				copy += "&quot;";
				offset = i + 1;
				break;
			case '\'':
				copy += str.substr(offset, i - offset);
				copy += "&apos;";
				offset = i + 1;
				break;
			case '<':
				copy += str.substr(offset, i - offset);
				copy += "&lt;";
				offset = i + 1;
				break;
			case '>':
				copy += str.substr(offset, i - offset);
				copy += "&gt;";
				offset = i + 1;
				break;
			case '&':
				copy += str.substr(offset, i - offset);
				copy += "&amp;";
				offset = i + 1;
				break;
			}
		}
		if (offset < str.size())
			copy += str.substr(offset);
		return copy;
	}

	static std::string XMLAttributesToString(const XMLElement& element)
	{
		std::string str = "";
		for (auto& [key, value] : element.attributes)
		{
			if (value.empty())
				continue;
			if (!str.empty())
				str += ' ';
			str += fmt::format("{}=\"{}\"", XMLEncode(key), XMLEncode(value));
		}
		return str;
	}

	XMLAttribute* XMLElement::findAttribute(std::string_view key)
	{
		for (auto& attribute : attributes)
			if (attribute.key == key)
				return &attribute;
		return nullptr;
	}

	XMLAttribute& XMLElement::getAttribute(std::string_view key)
	{
		for (auto& attribute : attributes)
			if (attribute.key == key)
				return attribute;
		auto& attrib = attributes.emplace_back();
		attrib.key   = key;
		return attrib;
	}

	const XMLAttribute* XMLElement::findAttribute(std::string_view key) const
	{
		for (auto& attribute : attributes)
			if (attribute.key == key)
				return &attribute;
		return nullptr;
	}

	std::string XMLToString(const XMLElement& element, std::size_t indent)
	{
		if (element.children.empty() && element.attributes.empty())
			return "";

		std::string indentStr     = std::string(indent, '\t');
		std::string attributesStr = XMLAttributesToString(element);
		if (element.children.empty() && attributesStr.empty())
			return "";
		if (!attributesStr.empty())
			attributesStr = ' ' + attributesStr;
		if (element.children.empty())
			return fmt::format("{}<{}{}/>", indentStr, XMLEncode(element.tag), attributesStr);

		std::string str = fmt::format("{}<{}{}>", indentStr, XMLEncode(element.tag), attributesStr);
		for (auto& child : element.children)
		{
			std::string childStr = XMLToString(child, indent + 1);
			if (!childStr.empty())
				str += fmt::format("\n{}", childStr);
		}
		return str += fmt::format("\n{}</{}>", indentStr, XMLEncode(element.tag));
	}

	std::string XMLToString(const XMLDocument& document)
	{
		std::string str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		return str += XMLToString(document.root);
	}
} // namespace Serialization