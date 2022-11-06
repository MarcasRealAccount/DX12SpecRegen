#pragma once

#include "Serialization/Serialization.h"

#include <string>
#include <vector>

namespace SDKInfo
{
	struct Constant
	{
	public:
		bool altered = false;

		std::string name;
		std::string type;
		std::string value;

	public:
		struct CustomOptionsFiller
		{
		public:
			template <class T, class TP>
			Serialization::IntegerOptions operator()([[maybe_unused]] T value, TP* parent)
			{
				Serialization::IntegerOptions options {};
				options.type   = parent->type;
				options.pad    = false;
				options.zeroes = false;
				return options;
			}
		};

		using StructInfo = Serialization::StructInfo<
		    "constant",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Constant::name>,
		        Serialization::FieldInfo<"altered", &Constant::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"type", &Constant::type, Serialization::PropertyInfo<Serialization::EInline::None /*, Serialization::EIntegerType::Int32*/>>,
		        Serialization::FieldInfo<"value", &Constant::value, Serialization::PropertyInfo<Serialization::EInline::None, nullptr, CustomOptionsFiller>>>>;
	};

	struct Ordinal
	{
	public:
		bool          isUnsigned() const;
		std::int64_t  int64() const;
		std::uint64_t uint64() const;

	public:
		bool altered = false;

		std::string                 name;
		Serialization::EIntegerType type;
		std::uint64_t               value;

	public:
		struct CustomOptionsFiller
		{
		public:
			template <class T, class TP>
			Serialization::IntegerOptions operator()([[maybe_unused]] T value, TP* parent)
			{
				Serialization::IntegerOptions options {};
				options.type   = parent->type;
				options.pad    = false;
				options.zeroes = false;
				return options;
			}
		};

		using StructInfo = Serialization::StructInfo<
		    "ordinal",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Ordinal::name>,
		        Serialization::FieldInfo<"altered", &Ordinal::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"value", &Ordinal::value, Serialization::PropertyInfo<Serialization::EInline::None, nullptr, CustomOptionsFiller>>>>;
	};

	struct Flag
	{
	public:
		bool          isUnsigned() const;
		std::int64_t  int64() const;
		std::uint64_t uint64() const;

	public:
		bool altered = false;

		std::string                 name;
		Serialization::EIntegerType type;
		std::uint64_t               value;

	public:
		struct CustomOptionsFiller
		{
		public:
			template <class T, class TP>
			Serialization::IntegerOptions operator()([[maybe_unused]] T value, TP* parent)
			{
				Serialization::IntegerOptions options {};
				options.type   = parent->type;
				options.pad    = true;
				options.zeroes = true;
				return options;
			}
		};

		using StructInfo = Serialization::StructInfo<
		    "flag",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Flag::name>,
		        Serialization::FieldInfo<"altered", &Flag::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"value", &Flag::value, Serialization::PropertyInfo<Serialization::EInline::None, nullptr, CustomOptionsFiller>>>>;
	};

	struct Enum
	{
	public:
		Ordinal* getOrdinal(std::string_view name);

	public:
		bool append  = false;
		bool altered = false;

		std::string                 name;
		Serialization::EIntegerType type;

		std::vector<Ordinal> ordinals;

	public:
		using StructInfo = Serialization::StructInfo<
		    "enum",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Enum::name>,
		        Serialization::FieldInfo<"altered", &Enum::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"append", &Enum::append, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"type", &Enum::type, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::EIntegerType::Int32>>,
		        Serialization::FieldInfo<"ordinals", &Enum::ordinals, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct Flags
	{
	public:
		Flag* getFlag(std::string_view name);

	public:
		bool append  = false;
		bool altered = false;

		std::string                 name;
		Serialization::EIntegerType type;

		std::vector<Flag> flags;

	public:
		using StructInfo = Serialization::StructInfo<
		    "flags",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Flags::name>,
		        Serialization::FieldInfo<"altered", &Flags::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"append", &Flags::append, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"type", &Flags::type, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::EIntegerType::Int32>>,
		        Serialization::FieldInfo<"flags", &Flags::flags, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct Variable
	{
	public:
		std::string name;
		std::string type;

	public:
		using StructInfo = Serialization::StructInfo<
		    "variable",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Variable::name>,
		        Serialization::FieldInfo<"type", &Variable::type>>>;
	};

	struct Union
	{
	public:
		Variable* getVariable(std::string_view name);

	public:
		bool altered = false;

		std::string name;

		std::vector<Variable> variables;

	public:
		using StructInfo = Serialization::StructInfo<
		    "union",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Union::name>,
		        Serialization::FieldInfo<"altered", &Union::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"variables", &Union::variables, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct Struct
	{
	public:
		Variable* getVariable(std::string_view name);

	public:
		bool altered = false;

		std::string name;

		std::vector<Variable> variables;

	public:
		using StructInfo = Serialization::StructInfo<
		    "struct",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Struct::name>,
		        Serialization::FieldInfo<"altered", &Struct::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"variables", &Struct::variables, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct GUID
	{
	public:
		bool altered = false;

		std::string name;
		std::string uuid;

	public:
		using StructInfo = Serialization::StructInfo<
		    "guid",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &GUID::name>,
		        Serialization::FieldInfo<"altered", &GUID::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"uuid", &GUID::uuid>>>;
	};

	struct Impl
	{
	public:
		bool        has = false;
		std::string lang;
		std::string code;

	public:
		using StructInfo = Serialization::StructInfo<
		    "impl",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"lang", &Impl::lang>,
		        Serialization::FieldInfo<"code", &Impl::code>>>;
	};

	struct Arg
	{
	public:
		std::string name;
		std::string type;
		std::string annotations;

	public:
		using StructInfo = Serialization::StructInfo<
		    "arg",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Arg::name>,
		        Serialization::FieldInfo<"type", &Arg::type, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "class" }>>,
		        Serialization::FieldInfo<"annotations", &Arg::annotations>>>;
	};

	struct TemplateArg
	{
	public:
		std::string name;
		std::string type;
		std::string concept_;

	public:
		using StructInfo = Serialization::StructInfo<
		    "template_arg",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &TemplateArg::name>,
		        Serialization::FieldInfo<"type", &TemplateArg::type, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "class" }>>,
		        Serialization::FieldInfo<"concept", &TemplateArg::concept_>>>;
	};

	struct CMethod
	{
	public:
		std::string name;
		std::string returnType;

		std::vector<Arg> args;

		Impl impl;

	public:
		using StructInfo = Serialization::StructInfo<
		    "cmethod",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &CMethod::name>,
		        Serialization::FieldInfo<"returnType", &CMethod::returnType, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "void" }>>,
		        Serialization::FieldInfo<"args", &CMethod::args, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"impl", &CMethod::impl>>>;
	};

	struct Method
	{
	public:
		std::string name;
		std::string returnType;
		bool        constexpr_ = false;

		std::vector<TemplateArg> templateArgs;
		std::vector<Arg>         args;

		Impl impl;

	public:
		using StructInfo = Serialization::StructInfo<
		    "method",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Method::name>,
		        Serialization::FieldInfo<"returnType", &Method::returnType, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "void" }>>,
		        Serialization::FieldInfo<"constexpr", &Method::constexpr_, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"templateArgs", &Method::templateArgs, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"args", &Method::args, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"impl", &Method::impl>>>;
	};

	struct CFunction
	{
	public:
		bool altered = false;

		std::string name;
		std::string returnType;

		std::vector<Arg> args;

	public:
		using StructInfo = Serialization::StructInfo<
		    "cfunction",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &CFunction::name>,
		        Serialization::FieldInfo<"altered", &CFunction::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"returnType", &CFunction::returnType, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "void" }>>,
		        Serialization::FieldInfo<"args", &CFunction::args, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct Function
	{
	public:
		bool altered = false;

		std::string name;
		std::string returnType;
		bool        constexpr_ = false;

		std::vector<TemplateArg> templateArgs;
		std::vector<Arg>         args;

		Impl impl;

	public:
		using StructInfo = Serialization::StructInfo<
		    "function",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Function::name>,
		        Serialization::FieldInfo<"altered", &Function::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"returnType", &Function::returnType, Serialization::PropertyInfo<Serialization::EInline::None, Serialization::TString { "void" }>>,
		        Serialization::FieldInfo<"constexpr", &Function::constexpr_, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"templateArgs", &Function::templateArgs, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"args", &Function::args, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"impl", &Function::impl>>>;
	};

	struct CInterface
	{
	public:
		bool     hasBase(std::string_view name);
		CMethod* getCMethod(std::string_view name);
		Method*  getMethod(std::string_view name);

	public:
		bool altered = false;

		std::string name;
		std::string uuid;

		std::vector<std::string> bases;
		std::vector<CMethod>     cMethods;
		std::vector<Method>      methods;

	public:
		using StructInfo = Serialization::StructInfo<
		    "cinterface",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &CInterface::name>,
		        Serialization::FieldInfo<"altered", &CInterface::altered, Serialization::PropertyInfo<Serialization::EInline::None, false>>,
		        Serialization::FieldInfo<"uuid", &CInterface::uuid>,
		        Serialization::FieldInfo<"bases", &CInterface::bases>,
		        Serialization::FieldInfo<"cmethods", &CInterface::cMethods, Serialization::PropertyInfo<Serialization::EInline::Inline>>,
		        Serialization::FieldInfo<"methods", &CInterface::methods, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	struct Header
	{
	public:
		Enum*       getEnum(std::string_view name);
		Flags*      getFlags(std::string_view name);
		Constant*   getConstant(std::string_view name);
		Union*      getUnion(std::string_view name);
		Struct*     getStruct(std::string_view name);
		GUID*       getGUID(std::string_view name);
		CInterface* getCInterface(std::string_view name);
		CFunction*  getCFunction(std::string_view name);
		Function*   getFunction(std::string_view name);

	public:
		std::string name;
		std::string namespace_;

		std::vector<Enum>       enums;
		std::vector<Flags>      flags;
		std::vector<Constant>   constants;
		std::vector<Union>      unions;
		std::vector<Struct>     structs;
		std::vector<GUID>       guids;
		std::vector<CInterface> cInterfaces;
		std::vector<CFunction>  cFunctions;
		std::vector<Function>   functions;

	public:
		using StructInfo = Serialization::StructInfo<
		    "header",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"name", &Header::name>,
		        Serialization::FieldInfo<"namespace", &Header::namespace_>,
		        Serialization::FieldInfo<"enums", &Header::enums>,
		        Serialization::FieldInfo<"flags", &Header::flags>,
		        Serialization::FieldInfo<"constants", &Header::constants>,
		        Serialization::FieldInfo<"unions", &Header::unions>,
		        Serialization::FieldInfo<"structs", &Header::structs>,
		        Serialization::FieldInfo<"guids", &Header::guids>,
		        Serialization::FieldInfo<"cinterfaces", &Header::cInterfaces>,
		        Serialization::FieldInfo<"cfunctions", &Header::cFunctions>,
		        Serialization::FieldInfo<"functions", &Header::functions>>>;
	};

	struct SDK
	{
	public:
		Header* getHeader(std::string_view name);

	public:
		std::string version;

		Header shared;

		std::vector<Header> headers;

	public:
		using StructInfo = Serialization::StructInfo<
		    "sdk",
		    Serialization::FieldInfos<
		        Serialization::FieldInfo<"version", &SDK::version>,
		        Serialization::FieldInfo<"shared", &SDK::shared, Serialization::PropertyInfo<Serialization::EInline::InlineRenamed>>,
		        Serialization::FieldInfo<"headers", &SDK::headers, Serialization::PropertyInfo<Serialization::EInline::Inline>>>>;
	};

	void RemovePreviousEnum(Enum& newer, Enum& older);
	void RemovePreviousFlags(Flags& newer, Flags& older);
	void RemovePreviousUnion(Union& newer, Union& older);
	void RemovePreviousStruct(Struct& newer, Struct& older);
	void RemovePreviousCInterface(CInterface& newer, CInterface& older);
	void RemovePreviousHeader(Header& newer, Header& older);
	void RemovePreviousSDK(SDK& newer, SDK& older);
} // namespace SDKInfo