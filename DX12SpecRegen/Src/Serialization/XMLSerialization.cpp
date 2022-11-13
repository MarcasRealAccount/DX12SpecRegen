#include "XMLSerialization.h"
#include "Utils/Unicode.h"

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

	static std::string XMLDecode(std::string_view str)
	{
		char        buf[4];
		std::string result;
		result.reserve(str.size());
		std::size_t offset = 0;
		for (std::size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] != '&')
				continue;

			result += str.substr(offset, i - offset);
			offset = i + 1;

			if (++i >= str.size())
			{
				result += '&';
				break;
			}

			char c = str[i];
			if (++i >= str.size())
			{
				result += c;
				break;
			}

			switch (c)
			{
			case '#': // Decode Unicode character
				if (str[i] == 'x' ||
				    str[i] == 'X')
				{
					std::size_t end = i + 1;
					while (end < str.size() && end != ';')
						++end;

					std::string_view hex = str.substr(i + 1, end - i - 1);
					std::uint32_t    v   = static_cast<std::uint32_t>(std::strtoull(hex.data(), nullptr, 16));
					std::uint8_t     len = Utils::Unicode::UTF32ToUTF8(static_cast<char32_t>(v), buf, 4);
					result += std::string_view { buf, buf + len };
				}
				else
				{
					std::size_t end = i + 1;
					while (end < str.size() && end != ';')
						++end;

					std::string_view hex = str.substr(i + 1, end - i - 1);
					std::uint32_t    v   = static_cast<std::uint32_t>(std::strtoull(hex.data(), nullptr, 10));
					std::uint8_t     len = Utils::Unicode::UTF32ToUTF8(static_cast<char32_t>(v), buf, 4);
					result += std::string_view { buf, buf + len };
				}
				break;
			case 'l':
				if (str.substr(i).starts_with("lt;"))
				{
					result += "<";
					offset += 4;
					break;
				}
				break;
			case 'g':
				if (str.substr(i).starts_with("gt;"))
				{
					result += ">";
					offset += 4;
					break;
				}
				break;
			case 'a':
				if (str.substr(i).starts_with("amp;"))
				{
					result += "&";
					offset += 5;
					break;
				}
				else if (str.substr(i).starts_with("apos;"))
				{
					result += "'";
					offset += 6;
					break;
				}
				break;
			case 'q':
				if (str.substr(i).starts_with("quot;"))
				{
					result += "\"";
					offset += 6;
					break;
				}
				break;
			}
		}
		if (offset < str.size())
			result += str.substr(offset);
		return result;
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
		std::string str = fmt::format("<?xml version=\"{}\" encoding=\"{}\"?>\n", document.version, document.encoding);
		return str += XMLToString(document.root);
	}

	static std::size_t SkipSpaces(std::string_view& str)
	{
		std::size_t offset = 0;
		while (offset < str.size())
		{
			std::uint8_t length    = 0;
			auto         codepoint = Utils::Unicode::UTF8ToUTF32(str.data() + offset, str.size() - offset, &length);
			if (!Utils::Unicode::Codepoint::IsWhitespace(codepoint))
				break;
			offset += length;
		}
		str = str.substr(offset);
		return offset;
	}

	static bool MatchCodepoint(std::string_view& str, Utils::Unicode::Codepoint::Codepoint codepoint)
	{
		std::uint8_t length = 0;
		if (Utils::Unicode::UTF8ToUTF32(str.data(), str.size(), &length) == codepoint)
		{
			str = str.substr(length);
			return true;
		}
		return false;
	}

	static std::size_t ParseIdentifier(std::string_view str, std::string& identifier)
	{
		std::size_t offset = 0;
		while (offset < str.size())
		{
			std::uint8_t length    = 0;
			auto         codepoint = Utils::Unicode::UTF8ToUTF32(str.data() + offset, str.size() - offset, &length);
			if (Utils::Unicode::Codepoint::IsWhitespace(codepoint) ||
			    codepoint == '>' ||
			    codepoint == '/' ||
			    codepoint == '=')
				break;
			offset += length;
		}
		identifier = str.substr(0, offset);
		return offset;
	}

	static std::size_t ParseString(std::string_view str, std::string& string)
	{
		std::string_view subView = str;
		if (!MatchCodepoint(subView, '"'))
			return 0;

		std::size_t offset = 0;
		while (offset < subView.size())
		{
			if (subView[offset] == '"')
				break;
			++offset;
		}
		subView = subView.substr(offset);
		if (!MatchCodepoint(subView, '"'))
			return 0;

		string = XMLDecode(str.substr(1, offset));
		return subView.data() - str.data();
	}

	static std::size_t ParseStartTag(std::string_view str, XMLElement& element, bool& closed)
	{
		std::string_view subView = str;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '<'))
			return 0;
		SkipSpaces(subView);
		if (subView.empty() || subView[0] == '/')
			return 0;
		std::size_t len = ParseIdentifier(subView, element.tag);
		if (len == 0)
			return 0;
		subView = subView.substr(len);

		while (!subView.empty())
		{
			if (SkipSpaces(subView) == 0)
				break;

			std::string attributeName;
			std::string attributeValue;
			len = ParseIdentifier(subView, attributeName);
			if (len == 0)
				break;
			subView = subView.substr(len);
			SkipSpaces(subView);
			if (!MatchCodepoint(subView, '='))
				return 0;
			SkipSpaces(subView);
			len = ParseString(subView, attributeValue);
			if (len == 0)
				return 0;
			subView = subView.substr(len);

			element.getAttribute(attributeName).value = attributeValue;
		}
		SkipSpaces(subView);
		if (MatchCodepoint(subView, '/'))
			closed = true;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '>'))
			return 0;

		return subView.data() - str.data();
	}

	static std::size_t ParseEndTag(std::string_view str, std::string& tagName)
	{
		std::string_view subView = str;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '<'))
			return 0;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '/'))
			return 0;
		std::size_t len = ParseIdentifier(subView, tagName);
		if (len == 0)
			return 0;
		subView = subView.substr(len);
		if (!MatchCodepoint(subView, '>'))
			return 0;
		return subView.data() - str.data();
	}

	static std::size_t ParseTag(std::string_view str, XMLElement& element)
	{
		std::string_view subView = str;
		bool             closed  = false;
		std::size_t      len     = ParseStartTag(subView, element, closed);
		if (len == 0)
			return 0;
		if (closed)
			return len;
		subView = subView.substr(len);

		while (!subView.empty())
		{
			XMLElement child;
			len = ParseTag(subView, child);
			if (len == 0)
				break;
			subView = subView.substr(len);
			element.children.emplace_back(std::move(child));
		}

		std::string tagName;
		len = ParseEndTag(subView, tagName);
		if (len == 0)
			return 0;
		if (tagName != element.tag)
			return 0;
		subView = subView.substr(len);
		return subView.data() - str.data();
	}

	static std::size_t ParseDeclaration(std::string_view str, XMLDocument& document)
	{
		std::string_view subView = str;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '<'))
			return 0;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '?'))
			return 0;

		XMLElement declaration;

		std::size_t len = ParseIdentifier(subView, declaration.tag);
		if (len == 0)
			return 0;
		if (declaration.tag != "xml")
			return 0;
		subView = subView.substr(len);

		while (!subView.empty())
		{
			if (SkipSpaces(subView) == 0)
				break;

			std::string attributeName;
			std::string attributeValue;
			len = ParseIdentifier(subView, attributeName);
			if (len == 0)
				break;
			subView = subView.substr(len);
			SkipSpaces(subView);
			if (!MatchCodepoint(subView, '='))
				return 0;
			SkipSpaces(subView);
			len = ParseString(subView, attributeValue);
			if (len == 0)
				return 0;
			subView = subView.substr(len);

			declaration.getAttribute(attributeName).value = attributeValue;
		}
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '?'))
			return 0;
		SkipSpaces(subView);
		if (!MatchCodepoint(subView, '>'))
			return 0;
		document.version  = declaration.getAttribute("version").value;
		document.encoding = declaration.getAttribute("encoding").value;
		return subView.data() - str.data();
	}

	XMLDocument StringToXML(std::string_view str)
	{
		XMLDocument      document {};
		std::string_view subView = str;
		SkipSpaces(subView);
		std::size_t len = ParseDeclaration(subView, document);
		subView         = subView.substr(len);
		ParseTag(subView, document.root);
		return document;
	}
} // namespace Serialization