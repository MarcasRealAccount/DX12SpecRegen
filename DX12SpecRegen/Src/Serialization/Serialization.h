#pragma once

#include <cstddef>

#include <concepts>
#include <string_view>
#include <utility>

namespace Serialization
{
	namespace Detail
	{
		template <bool B, class T, class F>
		struct TypeSelector
		{
		public:
			using Type = F;
		};

		template <class T, class F>
		struct TypeSelector<true, T, F>
		{
		public:
			using Type = T;
		};

		template <bool B, class T, class F>
		using TypeSelectorT = typename TypeSelector<B, T, F>::Type;

		template <class... Ts>
		struct Tuple;

		template <class T, class... Ts>
		struct Tuple<T, Ts...>
		{
		public:
			using Current = T;
			using Next    = Tuple<Ts...>;

			template <std::size_t N>
			using Nth = TypeSelectorT<N == 0, Current, typename Next::template Nth<N - 1>>;

			static constexpr std::size_t Size = sizeof...(Ts) + 1;
		};

		template <>
		struct Tuple<>
		{
		public:
			using Current = void;
			using Next    = Tuple<>;

			template <std::size_t N>
			using Nth = void;

			static constexpr std::size_t Size = 0;
		};

		template <auto TPtr>
		struct FieldProperties
		{
		public:
			using Class   = void;
			using Type    = void;
			using PtrType = void*;

			static constexpr PtrType Ptr = TPtr;
		};

		template <class C, class T, T C::*TPtr>
		struct FieldProperties<TPtr>
		{
		public:
			using Class   = C;
			using Type    = T;
			using PtrType = T C::*;

			static constexpr PtrType Ptr = TPtr;
		};
	} // namespace Detail

	template <std::size_t N>
	struct TString
	{
	public:
		static constexpr std::size_t Size = N - 1;

		constexpr explicit TString(const char* str) noexcept
		{
			std::size_t i = 0;
			for (; i < N; ++i)
			{
				char c = str[i];
				if (c == '\0')
					break;
				m_Buf[i] = c;
			}
			for (; i < N; ++i)
				m_Buf[i] = '\0';
		}
		template <std::size_t N2>
		constexpr TString(const char (&str)[N2]) noexcept
		{
			std::size_t smaller = std::min<std::size_t>(N, N2);
			if (smaller > 1)
			{
				--smaller;
				for (std::size_t i = 0; i < smaller; ++i)
					m_Buf[i] = str[i];
			}
			m_Buf[N - 1] = '\0';
		}
		constexpr operator const char*() const { return m_Buf; }
		constexpr operator std::string_view() const { return std::string_view { m_Buf, N - 1 }; }
		explicit  operator std::string() const { return std::string { m_Buf, N - 1 }; }

		friend bool operator==(const std::string& lhs, const TString& rhs) { return lhs == std::string_view { rhs }; }
		friend bool operator==(const TString& lhs, const std::string& rhs) { return std::string_view { lhs } == rhs; }

		char m_Buf[N] {};
	};
	template <std::size_t N>
	TString(const char (&)[N]) -> TString<N>;

	enum class EIntegerType
	{
		Bool,
		Char8,
		Char16,
		Char32,
		Int8,
		Int16,
		Int32,
		Int64,
		Int128,
		UInt8,
		UInt16,
		UInt32,
		UInt64,
		UInt128
	};

	constexpr bool IsUnsigned(EIntegerType type)
	{
		switch (type)
		{
		case EIntegerType::Bool: return true;
		case EIntegerType::Char8: return true;
		case EIntegerType::Char16: return true;
		case EIntegerType::Char32: return true;
		case EIntegerType::Int8: return false;
		case EIntegerType::Int16: return false;
		case EIntegerType::Int32: return false;
		case EIntegerType::Int64: return false;
		case EIntegerType::Int128: return false;
		case EIntegerType::UInt8: return true;
		case EIntegerType::UInt16: return true;
		case EIntegerType::UInt32: return true;
		case EIntegerType::UInt64: return true;
		case EIntegerType::UInt128: return true;
		}
		return false;
	}

	struct IntegerOptions
	{
	public:
		EIntegerType type;
		bool         pad;
		bool         zeroes;
	};

	struct DefaultIntegerOptionsFiller
	{
	public:
		template <class T, class TP>
		IntegerOptions operator()(T value, [[maybe_unused]] TP* parent)
		{
			IntegerOptions options {};
			if constexpr (std::same_as<T, std::int8_t>)
				options.type = EIntegerType::Int8;
			else if constexpr (std::same_as<T, std::int16_t>)
				options.type = EIntegerType::Int16;
			else if constexpr (std::same_as<T, std::int32_t>)
				options.type = EIntegerType::Int32;
			else if constexpr (std::same_as<T, std::int64_t>)
				options.type = EIntegerType::Int64;
			else if constexpr (std::same_as<T, std::uint8_t>)
				options.type = EIntegerType::UInt8;
			else if constexpr (std::same_as<T, std::uint16_t>)
				options.type = EIntegerType::UInt16;
			else if constexpr (std::same_as<T, std::uint32_t>)
				options.type = EIntegerType::UInt32;
			else if constexpr (std::same_as<T, std::uint64_t>)
				options.type = EIntegerType::UInt64;
			options.pad    = false;
			options.zeroes = false;
			return options;
		}
	};

	enum class EInline
	{
		None = 0,
		Inline,
		InlineRenamed
	};

	template <EInline TInline = EInline::None, auto TDefault = nullptr, class TIntegerOptionsFiller = DefaultIntegerOptionsFiller>
	struct PropertyInfo
	{
	public:
		static constexpr EInline Inline  = TInline;
		static constexpr auto    Default = TDefault;
		using IntegerOptionsFiller       = TIntegerOptionsFiller;
	};

	template <class T, class U>
	concept IsDefaultable = !
	std::is_null_pointer_v<T>;

	template <TString TName, auto TPtr, class TProperties = PropertyInfo<>>
	struct FieldInfo
	{
	public:
		using FieldProperties = Detail::FieldProperties<TPtr>;
		using Class           = typename FieldProperties::Class;
		using Type            = typename FieldProperties::Type;
		using PtrType         = typename FieldProperties::PtrType;

		static constexpr auto    Name = TName;
		static constexpr PtrType Ptr  = TPtr;

		using Properties = TProperties;

	public:
		static Type&       Get(Class& object) { return object.*Ptr; }
		static const Type& Get(const Class& object) { return object.*Ptr; }
		template <std::assignable_from<Type> U>
		static Type& Set(Class& object, const U& copy)
		    requires std::assignable_from<Type, const U&>
		{
			return object.*Ptr = copy;
		}
		template <std::assignable_from<Type> U>
		static Type& Set(Class& object, U&& move)
		    requires std::assignable_from<Type, U&&>
		{
			return object.*Ptr = std::move(move);
		}
	};

	template <class... TFields>
	struct FieldInfos
	{
	public:
		using Fields = Detail::Tuple<TFields...>;
	};

	template <TString TName, class TFields>
	struct StructInfo
	{
	public:
		using Fields = typename TFields::Fields;

		static constexpr auto Name = TName;
	};

	template <class T>
	concept Serializable =
	    requires {
		    typename T::StructInfo;
	    };
} // namespace Serialization