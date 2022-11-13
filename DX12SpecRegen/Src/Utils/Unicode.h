#pragma once

#include <cstddef>
#include <cstdint>

#include <string_view>

namespace Utils::Unicode
{
	constexpr std::uint8_t UTF8Length(std::uint8_t c)
	{
		if (c <= 0x7F)
			return 1;
		else if (c <= 0xBF)
			return 0;
		else if (c <= 0xDF)
			return 2;
		else if (c <= 0xEF)
			return 3;
		else
			return 4;
	}

	constexpr std::uint8_t UTF8Length(char32_t codepoint)
	{
		if (codepoint <= 0x7F)
			return 1;
		else if (codepoint <= 0x7FF)
			return 2;
		else if (codepoint <= 0xFFFF)
			return 3;
		else if (codepoint <= 0x10'FFFF)
			return 4;
		else
			return 0;
	}

	constexpr std::uint8_t UTF16Length(std::uint16_t c)
	{
		auto x = c & 0xFC00;
		switch (x)
		{
		case 0xD800: return 2;
		case 0xDC00: return 0;
		default: return 1;
		}
	}

	constexpr std::uint8_t UTF16Length(char32_t codepoint)
	{
		if (codepoint <= 0xFFFF)
			return 1;
		else if (codepoint <= 0x10'FFFF)
			return 2;
		return 0;
	}

	constexpr char32_t UTF8ToUTF32(const char* utf8, std::size_t size, std::uint8_t* pCodepointLength)
	{
		if (!utf8 || !size)
		{
			if (pCodepointLength) *pCodepointLength = 0;
			return 0;
		}

		std::uint8_t len     = UTF8Length(static_cast<std::uint8_t>(utf8[0]));
		std::uint8_t realLen = len;
		std::uint8_t buf[4] { static_cast<std::uint8_t>(utf8[0]), 0, 0, 0 };
		for (std::uint8_t i = 1; i < len; ++i)
		{
			buf[i] = i < size ? static_cast<std::uint8_t>(utf8[i]) : 0;
			if ((buf[i] & 0xC0) != 0x80)
			{
				realLen = i;
				break;
			}
		}
		if (pCodepointLength) *pCodepointLength = realLen;
		switch (len)
		{
		case 0: return 0;
		case 1: return buf[0];
		case 2: return (buf[0] & 0x1F) << 6 | (buf[1] & 0x3F);
		case 3: return (buf[0] & 0xF) << 12 | (buf[1] & 0x3F) << 6 | (buf[2] & 0x3F);
		case 4: return (buf[0] & 0x7) << 18 | (buf[1] & 0x3F) << 12 | (buf[2] & 0x3F) << 6 | (buf[3] & 0x3F);
		default: return 0;
		}
	}

	constexpr std::uint8_t UTF32ToUTF8(char32_t utf32, char* utf8, std::size_t size)
	{
		if (!utf8 || !size)
			return 0;

		std::uint8_t len = UTF8Length(utf32);
		if (size < len)
			return 0;
		switch (len)
		{
		case 0:
			return 0;
		case 1:
			utf8[0] = static_cast<std::uint8_t>(utf32);
			return 1;
		case 2:
			utf8[0] = 0xC0 | static_cast<std::uint8_t>((utf32 >> 6) & 0x1F);
			utf8[1] = 0x80 | static_cast<std::uint8_t>(utf32 & 0x3F);
			return 2;
		case 3:
			utf8[0] = 0xC0 | static_cast<std::uint8_t>((utf32 >> 12) & 0xF);
			utf8[1] = 0x80 | static_cast<std::uint8_t>((utf32 >> 6) & 0x3F);
			utf8[2] = 0x80 | static_cast<std::uint8_t>(utf32 & 0x3F);
			return 3;
		case 4:
			utf8[0] = 0xC0 | static_cast<std::uint8_t>((utf32 >> 18) & 0x7);
			utf8[1] = 0x80 | static_cast<std::uint8_t>((utf32 >> 12) & 0x3F);
			utf8[2] = 0x80 | static_cast<std::uint8_t>((utf32 >> 6) & 0x3F);
			utf8[3] = 0x80 | static_cast<std::uint8_t>(utf32 & 0x3F);
			return 4;
		default:
			return 0;
		}
	}

	constexpr char32_t UTF16ToUTF32(const wchar_t* utf16, std::size_t size, std::uint8_t* pCodepointLength)
	{
		if (!utf16 || !size)
		{
			if (pCodepointLength) *pCodepointLength = 0;
			return 0;
		}

		std::uint8_t  len     = UTF16Length(static_cast<std::uint16_t>(utf16[0]));
		std::uint8_t  realLen = len;
		std::uint16_t buf[2] { static_cast<std::uint16_t>(utf16[0]), 0 };
		for (std::uint8_t i = 1; i < len; ++i)
		{
			buf[i] = i < size ? static_cast<std::uint16_t>(utf16[i]) : 0;
			if ((buf[i] & 0xFC00) != 0xDC00)
			{
				realLen = i;
				break;
			}
		}
		if (pCodepointLength) *pCodepointLength = realLen;
		switch (len)
		{
		case 0: return 0;
		case 1: return buf[0];
		case 2: return 0x1'0000 + ((buf[0] & 0x3FF) << 10 | buf[1] & 0x3FF);
		default: return 0;
		}
	}

	constexpr std::uint8_t UTF32ToUTF16(char32_t utf32, wchar_t* utf16, std::size_t size)
	{
		if (!utf16 || !size)
			return 0;

		std::uint8_t len = UTF16Length(utf32);
		if (size < len)
			return 0;
		switch (len)
		{
		case 0:
			return 0;
		case 1:
			utf16[0] = static_cast<std::uint16_t>(utf32);
			return 1;
		case 2:
		{
			std::uint32_t U = utf32 - 0x1'0000;
			utf16[0]        = 0xD800 | (U >> 10) & 0x3FF;
			utf16[1]        = 0xDC00 | (U >> 10) & 0x3FF;
			return 2;
		}
		default:
			return 0;
		}
	}

	constexpr std::size_t ConvertUTF8ToUTF32(const char* utf8, std::size_t utf8Size, char32_t* utf32, std::size_t utf32Size, std::size_t& utf8Offset)
	{
		std::size_t utf32Offset = 0;

		std::uint8_t len = 0;
		utf8Offset       = 0;
		for (; utf8Offset < utf8Size && utf32Offset < utf32Size; ++utf32Offset)
		{
			utf32[utf32Offset] = UTF8ToUTF32(utf8 + utf8Offset, utf8Size - utf8Offset, &len);
			utf8Offset += len;
		}

		return utf32Offset;
	}

	constexpr std::size_t ConvertUTF32ToUTF8(const char32_t* utf32, std::size_t utf32Size, char* utf8, std::size_t utf8Size, std::size_t& utf8Offset)
	{
		std::size_t utf32Offset = 0;

		utf8Offset = 0;
		for (; utf8Offset < utf8Size && utf32Offset < utf32Size; ++utf32Offset)
			utf8Offset += UTF32ToUTF8(utf32[utf32Offset], utf8 + utf8Offset, utf8Size - utf8Offset);
		return utf32Offset;
	}

	constexpr std::size_t ConvertUTF16ToUTF32(const wchar_t* utf16, std::size_t utf16Size, char32_t* utf32, std::size_t utf32Size, std::size_t& utf16Offset)
	{
		std::size_t utf32Offset = 0;

		std::uint8_t len = 0;
		utf16Offset      = 0;
		for (; utf16Offset < utf16Size && utf32Offset < utf32Size; ++utf32Offset)
		{
			utf32[utf32Offset] = UTF16ToUTF32(utf16 + utf16Offset, utf16Size - utf16Offset, &len);
			utf16Offset += len;
		}

		return utf32Offset;
	}

	constexpr std::size_t ConvertUTF32ToUTF16(const char32_t* utf32, std::size_t utf32Size, wchar_t* utf16, std::size_t size, std::size_t& utf16Offset)
	{
		std::size_t utf32Offset = 0;

		utf16Offset = 0;
		for (; utf16Offset < size && utf32Offset < utf32Size; ++utf32Offset)
			utf16Offset += UTF32ToUTF16(utf32[utf32Offset], utf16 + utf16Offset, size - utf16Offset);
		return utf32Offset;
	}

	constexpr std::size_t ConvertUTF8ToUTF16(const char* utf8, std::size_t utf8Size, wchar_t* utf16, std::size_t utf16Size, std::size_t& utf8Offset)
	{
		std::uint8_t utf8Len     = 0;
		std::size_t  utf16Offset = 0;
		utf8Offset               = 0;
		for (; utf8Offset < utf8Size && utf16Offset < utf16Size;)
		{
			std::uint32_t codepoint = UTF8ToUTF32(utf8 + utf8Offset, utf8Size - utf8Offset, &utf8Len);
			utf16Offset += UTF32ToUTF16(codepoint, utf16 + utf16Offset, utf16Size - utf16Offset);
			utf8Offset += utf8Len;
		}
		return utf16Offset;
	}

	constexpr std::size_t ConvertUTF16ToUTF8(const wchar_t* utf16, std::size_t utf16Size, char* utf8, std::size_t utf8Size, std::size_t& utf16Offset)
	{
		std::uint8_t utf16Len   = 0;
		std::size_t  utf8Offset = 0;
		utf16Offset             = 0;
		for (; utf8Offset < utf8Size && utf16Offset < utf16Size;)
		{
			std::uint32_t codepoint = UTF16ToUTF32(utf16 + utf16Offset, utf16Size - utf16Offset, &utf16Len);
			utf8Offset += UTF32ToUTF8(codepoint, utf8 + utf8Offset, utf8Size - utf8Offset);
			utf16Offset += utf16Len;
		}
		return utf8Offset;
	}

	constexpr std::size_t UTF8ToUTF32RequiredLength(const char* utf8, std::size_t utf8Size)
	{
		std::size_t requiredLength = 0;
		for (std::size_t offset = 0; offset < utf8Size; ++requiredLength)
		{
			auto len = UTF8Length(static_cast<std::uint8_t>(utf8[offset]));
			if (len == 0)
				break;
			offset += len;
		}
		return requiredLength;
	}

	constexpr std::size_t UTF32ToUTF8RequiredLength(const char32_t* utf32, std::size_t utf32Size)
	{
		std::size_t requiredLength = 0;
		for (std::size_t offset = 0; offset < utf32Size; ++offset)
		{
			auto len = UTF8Length(utf32[offset]);
			if (len == 0)
				break;
			requiredLength += len;
		}
		return requiredLength;
	}

	constexpr std::size_t UTF8ToUTF16RequiredLength(const char* utf8, std::size_t utf8Size)
	{
		std::size_t requiredLength = 0;
		for (std::size_t offset = 0; offset < utf8Size;)
		{
			auto len = UTF8Length(static_cast<std::uint8_t>(utf8[offset]));
			if (len == 0)
				break;
			else if (len <= 3)
				++requiredLength;
			else
				requiredLength += 2;
			offset += len;
		}
		return requiredLength;
	}

	constexpr std::size_t UTF16ToUTF8RequiredLength(const wchar_t* utf16, std::size_t utf16Size)
	{
		std::size_t requiredLength = 0;
		for (std::size_t offset = 0; offset < utf16Size;)
		{
			auto len = UTF16Length(static_cast<std::uint16_t>(utf16[offset]));
			if (len == 0)
			{
				break;
			}
			else if (len == 1)
			{
				wchar_t c = utf16[offset];
				if (c <= 0x7F)
					++requiredLength;
				else if (c <= 0x7FF)
					requiredLength += 2;
				else
					requiredLength += 3;
			}
			else
			{
				requiredLength += 4;
			}
			offset += len;
		}
		return requiredLength;
	}

	inline std::u32string ConvertUTF8ToUTF32(std::string_view utf8)
	{
		std::u32string utf32(UTF8ToUTF32RequiredLength(utf8.data(), utf8.size()), '\0');
		std::size_t    utf8Offset = 0;
		ConvertUTF8ToUTF32(utf8.data(), utf8.size(), utf32.data(), utf32.size(), utf8Offset);
		return utf32;
	}

	inline std::string ConvertUTF32ToUTF8(std::u32string_view utf32)
	{
		std::string utf8(UTF32ToUTF8RequiredLength(utf32.data(), utf32.size()), '\0');
		std::size_t utf32Offset = 0;
		ConvertUTF32ToUTF8(utf32.data(), utf32.size(), utf8.data(), utf8.size(), utf32Offset);
		return utf8;
	}

	inline std::u16string ConvertUTF8ToUTF16(std::string_view utf8)
	{
		std::u16string utf16(UTF8ToUTF16RequiredLength(utf8.data(), utf8.size()), '\0');
		std::size_t    utf8Offset = 0;
		ConvertUTF8ToUTF16(utf8.data(), utf8.size(), reinterpret_cast<wchar_t*>(utf16.data()), utf16.size(), utf8Offset);
		return utf16;
	}

	inline std::string ConvertUTF16ToUTF8(std::u16string_view utf16)
	{
		std::string utf8(UTF16ToUTF8RequiredLength(reinterpret_cast<const wchar_t*>(utf16.data()), utf16.size()), '\0');
		std::size_t utf16Offset = 0;
		ConvertUTF16ToUTF8(reinterpret_cast<const wchar_t*>(utf16.data()), utf16.size(), utf8.data(), utf8.size(), utf16Offset);
		return utf8;
	}

	inline std::wstring ConvertUTF8ToWide(std::string_view utf8)
	{
		std::wstring wide(UTF8ToUTF16RequiredLength(utf8.data(), utf8.size()), '\0');
		std::size_t  utf8Offset = 0;
		ConvertUTF8ToUTF16(utf8.data(), utf8.size(), wide.data(), wide.size(), utf8Offset);
		return wide;
	}

	inline std::string ConvertWideToUTF8(std::wstring_view wide)
	{
		std::string utf8(UTF16ToUTF8RequiredLength(wide.data(), wide.size()), '\0');
		std::size_t utf16Offset = 0;
		ConvertUTF16ToUTF8(wide.data(), wide.size(), utf8.data(), utf8.size(), utf16Offset);
		return utf8;
	}

	namespace Codepoint
	{
		struct Codepoint
		{
		public:
			Codepoint(char c) : m_Codepoint(c) {}
			Codepoint(char8_t c) : m_Codepoint(c) {}
			Codepoint(char16_t c) : m_Codepoint(c) {}
			Codepoint(char32_t c) : m_Codepoint(c) {}
			Codepoint(wchar_t c) : m_Codepoint(c) {}

			template <std::size_t N>
			Codepoint(const char (&c)[N])
			    : m_Codepoint(UTF8ToUTF32(c, N, nullptr))
			{
			}
			template <std::size_t N>
			Codepoint(const char8_t (&c)[N])
			    : m_Codepoint(UTF8ToUTF32(c, N, nullptr))
			{
			}
			template <std::size_t N>
			Codepoint(const char16_t (&c)[N])
			    : m_Codepoint(UTF16ToUTF32(c, N, nullptr))
			{
			}
			template <std::size_t N>
			Codepoint(const wchar_t (&c)[N])
			    : m_Codepoint(UTF16ToUTF32(c, N, nullptr))
			{
			}

			operator char32_t() const { return m_Codepoint; }

			char32_t m_Codepoint;
		};

		inline bool IsWhitespace(Codepoint codepoint)
		{
			switch (codepoint)
			{
			case 0x0009:
			case 0x000A:
			case 0x000B:
			case 0x000C:
			case 0x000D:
			case 0x0020:
			case 0x0085:
			case 0x00A0:
			case 0x1680:
			case 0x2000:
			case 0x2001:
			case 0x2002:
			case 0x2003:
			case 0x2004:
			case 0x2005:
			case 0x2006:
			case 0x2007:
			case 0x2008:
			case 0x2009:
			case 0x200A:
			case 0x2028:
			case 0x2029:
			case 0x202F:
			case 0x205F:
			case 0x3000:
				return true;
			default:
				return false;
			}
		}
	} // namespace Codepoint
} // namespace Utils::Unicode