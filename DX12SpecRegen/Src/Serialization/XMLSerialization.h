#pragma once

#include "Serialization.h"

#include <spdlog/fmt/fmt.h>

namespace Serialization
{
	struct XMLAttribute
	{
	public:
		std::string key;
		std::string value;
	};

	struct XMLElement
	{
	public:
		XMLAttribute*       findAttribute(std::string_view key);
		XMLAttribute&       getAttribute(std::string_view key);
		const XMLAttribute* findAttribute(std::string_view key) const;

	public:
		std::string tag;

		std::vector<XMLAttribute> attributes;
		std::vector<XMLElement>   children;
	};

	struct XMLDocument
	{
	public:
		XMLElement root;
	};

	std::string XMLToString(const XMLElement& element, std::size_t indent = 0);
	std::string XMLToString(const XMLDocument& document);

	template <class T>
	struct XMLSerializer;

	template <class T>
	struct XMLSerializerInfo
	{
	public:
		static constexpr bool AsAttribute = []() -> bool
		{
			if constexpr (requires { T::AsAttribute; })
				return T::AsAttribute;
			else
				return false;
		}();
	};

	template <class T>
	void XMLSerialiaze(const T& value, XMLElement& element)
	{
		using Serializer = XMLSerializer<T>;
		Serializer serializer {};
		serializer.Serialize(value, element);
	}

	template <class T>
	void XMLDeserialize(T& value, const XMLElement& element)
	{
		using Serializer = XMLSerializer<T>;
		Serializer serializer {};
		serializer.Deserialize(value, element);
	}

	template <Serializable T>
	struct XMLSerializer<T>
	{
	public:
		using StructInfo = typename T::StructInfo;

	public:
		template <class TFieldInfo>
		void SerializeField(const T& value, XMLElement& element)
		{
			using FieldSerializer     = XMLSerializer<typename TFieldInfo::Type>;
			using FieldSerializerInfo = XMLSerializerInfo<FieldSerializer>;
			using FieldProperties     = typename TFieldInfo::Properties;

			static constexpr bool B = IsDefaultable<decltype(false), bool>;

			if constexpr (IsDefaultable<decltype(FieldProperties::Default), typename TFieldInfo::Type>)
				if (TFieldInfo::Get(value) == FieldProperties::Default)
					return;

			FieldSerializer serializer {};
			if constexpr (FieldSerializerInfo::AsAttribute)
			{
				XMLAttribute& attrib = element.getAttribute(TFieldInfo::Name);
				serializer.Serialize(TFieldInfo::Get(value), attrib.value);
			}
			else
			{
				XMLElement* fieldElement = nullptr;
				if constexpr (FieldProperties::Inline == EInline::Inline)
				{
					fieldElement = &element;
				}
				else if constexpr (FieldProperties::Inline == EInline::InlineRenamed)
				{
					fieldElement      = &element.children.emplace_back();
					fieldElement->tag = TFieldInfo::Name;
				}
				else
				{
					fieldElement = &element.children.emplace_back();
					if constexpr (Serializable<typename TFieldInfo::Type>)
						fieldElement->tag = TFieldInfo::Type::StructInfo::Name;
					else
						fieldElement->tag = TFieldInfo::Name;
				}
				serializer.Serialize(TFieldInfo::Get(value), *fieldElement);
			}
		}

		template <class... TFieldInfos>
		void SerializeFields(const T& value, XMLElement& element, Detail::Tuple<TFieldInfos...>)
		{
			(SerializeField<TFieldInfos>(value, element), ...);
		}

		template <class TFieldInfo>
		void DeserializeField(T& value, const XMLElement& element)
		{
			using FieldSerializer     = XMLSerializer<typename TFieldInfo::Type>;
			using FieldSerializerInfo = XMLSerializerInfo<FieldSerializer>;
			using FieldProperties     = typename TFieldInfo::Properties;

			FieldSerializer serializer {};
			if constexpr (FieldSerializerInfo::AsAttribute)
			{
				auto attrib = element.findAttribute(TFieldInfo::Name);
				if (attrib)
					serializer.Deserialize(TFieldInfo::Get(value), attrib->value);
				else if constexpr (IsDefaultable<decltype(FieldProperties::Default), typename TFieldInfo::Type>)
					TFieldInfo::Set(value, FieldProperties::Default);
			}
			else
			{
				XMLElement* fieldElement = nullptr;
				if constexpr (FieldProperties::Inline)
				{
					fieldElement = &element;
				}
				else if constexpr (FieldProperties::Inline == EInline::InlineRenamed)
				{
					auto elementItr = std::find_if(element.children.begin(), element.children.end(),
					                               [](const XMLElement& child) -> bool
					                               {
						                               if constexpr (Serializable<typename TFieldInfo::Type>)
							                               return child.tag == TFieldInfo::Type::StructInfo::Name;
						                               else
							                               return child.tag == TFieldInfo::Name;
					                               });
					if (elementItr == element.children.end())
					{
						if constexpr (IsDefaultable<decltype(FieldProperties::Default), typename TFieldInfo::Type>)
						{
							TFieldInfo::Set(value, FieldProperties::Default);
							return;
						}
					}
					else
					{
						fieldElement      = &(*elementItr);
						fieldElement->tag = TFieldInfo::Name;
					}
				}
				else
				{
					auto elementItr = std::find_if(element.children.begin(), element.children.end(),
					                               [](const XMLElement& child) -> bool
					                               {
						                               if constexpr (Serializable<typename TFieldInfo::Type>)
							                               return child.tag == TFieldInfo::Type::StructInfo::Name;
						                               else
							                               return child.tag == TFieldInfo::Name;
					                               });
					if (elementItr == element.children.end())
					{
						if constexpr (IsDefaultable<decltype(FieldProperties::Default), typename TFieldInfo::Type>)
						{
							TFieldInfo::Set(value, FieldProperties::Default);
							return;
						}
					}
					else
					{
						fieldElement = &(*elementItr);
						if constexpr (Serializable<typename TFieldInfo::Type>)
							fieldElement->tag = TFieldInfo::Type::StructInfo::Name;
						else
							fieldElement->tag = TFieldInfo::Name;
					}
				}
				serializer.Deserialize(TFieldInfo::Get(value), *fieldElement);
			}
		}

		template <class... TFieldInfos>
		void DeserializeFields(T& value, const XMLElement& element, Detail::Tuple<TFieldInfos...>)
		{
			(DeserializeField<TFieldInfos>(value, element), ...);
		}

		void Serialize(const T& value, XMLElement& element)
		{
			SerializeFields(value, element, typename StructInfo::Fields {});
		}

		void Deserialize(T& value, const XMLElement& element)
		{
			DeserializeFields(value, element, typename StructInfo::Fields {});
		}
	};

	template <>
	struct XMLSerializer<std::int8_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::int8_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::int8_t& value, const std::string& attribute)
		{
			value = static_cast<std::int8_t>(std::strtoll(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::int16_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::int16_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::int16_t& value, const std::string& attribute)
		{
			value = static_cast<std::int16_t>(std::strtoll(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::int32_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::int32_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::int32_t& value, const std::string& attribute)
		{
			value = static_cast<std::int32_t>(std::strtoll(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::int64_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::int64_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::int64_t& value, const std::string& attribute)
		{
			value = std::strtoll(attribute.c_str(), nullptr, 10);
		}
	};

	template <>
	struct XMLSerializer<std::uint8_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::uint8_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::uint8_t& value, const std::string& attribute)
		{
			value = static_cast<std::uint8_t>(std::strtoull(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::uint16_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::uint16_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::uint16_t& value, const std::string& attribute)
		{
			value = static_cast<std::uint16_t>(std::strtoull(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::uint32_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::uint32_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::uint32_t& value, const std::string& attribute)
		{
			value = static_cast<std::uint32_t>(std::strtoull(attribute.c_str(), nullptr, 10));
		}
	};

	template <>
	struct XMLSerializer<std::uint64_t>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(std::uint64_t value, std::string& attribute)
		{
			attribute = fmt::format("{}", value);
		}

		void Deserialize(std::uint64_t& value, const std::string& attribute)
		{
			value = std::strtoull(attribute.c_str(), nullptr, 10);
		}
	};

	template <>
	struct XMLSerializer<EIntegerType>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(EIntegerType value, std::string& attribute)
		{
			switch (value)
			{
			case EIntegerType::Bool: attribute = "b"; break;
			case EIntegerType::Char8: attribute = "c8"; break;
			case EIntegerType::Char16: attribute = "c16"; break;
			case EIntegerType::Char32: attribute = "c32"; break;
			case EIntegerType::Int8: attribute = "i8"; break;
			case EIntegerType::Int16: attribute = "i16"; break;
			case EIntegerType::Int32: attribute = "i32"; break;
			case EIntegerType::Int64: attribute = "i64"; break;
			case EIntegerType::Int128: attribute = "i128"; break;
			case EIntegerType::UInt8: attribute = "u8"; break;
			case EIntegerType::UInt16: attribute = "u16"; break;
			case EIntegerType::UInt32: attribute = "u32"; break;
			case EIntegerType::UInt64: attribute = "u64"; break;
			case EIntegerType::UInt128: attribute = "u128"; break;
			default: attribute = "WTF ARE YOU DOING BRO"; break;
			}
		}

		void Deserialize(EIntegerType& value, const std::string& attribute)
		{
			if (attribute.empty())
			{
				value = EIntegerType::Int32;
				return;
			}

			switch (attribute[0])
			{
			case 'b':
				value = EIntegerType::Bool;
				break;
			case 'c':
			{
				std::uint64_t width = std::strtoull(attribute.c_str() + 1, nullptr, 10);
				switch (width)
				{
				case 8: value = EIntegerType::Char8; break;
				case 16: value = EIntegerType::Char16; break;
				case 32: value = EIntegerType::Char32; break;
				default: value = EIntegerType::Char8; break;
				}
				break;
			}
			case 'i':
			{
				std::uint64_t width = std::strtoull(attribute.c_str() + 1, nullptr, 10);
				switch (width)
				{
				case 8: value = EIntegerType::Int8; break;
				case 16: value = EIntegerType::Int16; break;
				case 32: value = EIntegerType::Int32; break;
				case 64: value = EIntegerType::Int64; break;
				case 128: value = EIntegerType::Int128; break;
				default: value = EIntegerType::Int32; break;
				}
				break;
			}
			case 'u':
			{
				std::uint64_t width = std::strtoull(attribute.c_str() + 1, nullptr, 10);
				switch (width)
				{
				case 8: value = EIntegerType::UInt8; break;
				case 16: value = EIntegerType::UInt16; break;
				case 32: value = EIntegerType::UInt32; break;
				case 64: value = EIntegerType::UInt64; break;
				case 128: value = EIntegerType::UInt128; break;
				default: value = EIntegerType::UInt32; break;
				}
				break;
			}
			default:
				value = EIntegerType::Int32;
				break;
			}
		}
	};

	template <>
	struct XMLSerializer<bool>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(bool value, std::string& attribute)
		{
			attribute = value ? "true" : "false";
		}

		void Deserialize(bool& value, const std::string& attribute)
		{
			value = attribute == "true";
		}
	};

	template <>
	struct XMLSerializer<std::string>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(const std::string& value, std::string& attribute)
		{
			attribute = value;
		}

		void Deserialize(std::string& value, const std::string& attribute)
		{
			value = attribute;
		}
	};

	template <>
	struct XMLSerializer<std::vector<std::string>>
	{
	public:
		static constexpr bool AsAttribute = true;

	public:
		void Serialize(const std::vector<std::string>& value, std::string& attribute)
		{
			for (std::size_t i = 0; i < value.size(); ++i)
			{
				if (i > 0)
					attribute += ';';
				attribute += value[i];
			}
		}

		void Deserialize(std::vector<std::string>& value, const std::string& attribute)
		{
			std::size_t offset = 0;
			while (offset < attribute.size())
			{
				std::size_t end = attribute.find_first_of(';', offset);
				if (end > attribute.size())
				{
					value.emplace_back(attribute.substr(offset));
					break;
				}
				value.emplace_back(attribute.substr(offset, end - offset));
				offset = end + 1;
			}
		}
	};

	template <Serializable T, class Alloc>
	struct XMLSerializer<std::vector<T, Alloc>>
	{
	public:
		void Serialize(const std::vector<T, Alloc>& value, XMLElement& element)
		{
			using StructInfo      = typename T::StructInfo;
			using FieldSerializer = XMLSerializer<T>;
			for (auto& val : value)
			{
				XMLElement&     elem = element.children.emplace_back();
				FieldSerializer serializer {};
				elem.tag = StructInfo::Name;
				serializer.Serialize(val, elem);
			}
		}

		void Deserialize(std::vector<T, Alloc>& value, const XMLElement& element)
		{
			using StructInfo      = typename T::StructInfo;
			using FieldSerializer = XMLSerializer<T>;
			for (auto& child : element.children)
			{
				if (child.tag == StructInfo.Name)
				{
					auto&           val = value.emplace_back();
					FieldSerializer serializer {};
					serializer.Deserialize(val, child);
				}
			}
		}
	};
} // namespace Serialization