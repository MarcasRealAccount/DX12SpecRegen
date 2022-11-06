#include "SDKInfo.h"

#include <spdlog/fmt/fmt.h>

#include <cstdlib>

namespace SDKInfo
{
	bool Ordinal::isUnsigned() const
	{
		return Serialization::IsUnsigned(type);
	}

	std::int64_t Ordinal::int64() const
	{
		return static_cast<std::int64_t>(value);
	}

	std::uint64_t Ordinal::uint64() const
	{
		return value;
	}

	bool Flag::isUnsigned() const
	{
		return Serialization::IsUnsigned(type);
	}

	std::int64_t Flag::int64() const
	{
		return static_cast<std::int64_t>(value);
	}

	std::uint64_t Flag::uint64() const
	{
		return value;
	}

	Ordinal* Enum::getOrdinal(std::string_view name)
	{
		for (auto& ordinalInfo : ordinals)
			if (ordinalInfo.name == name)
				return &ordinalInfo;
		return nullptr;
	}

	Flag* Flags::getFlag(std::string_view name)
	{
		for (auto& flagInfo : flags)
			if (flagInfo.name == name)
				return &flagInfo;
		return nullptr;
	}

	Variable* Union::getVariable(std::string_view name)
	{
		for (auto& variableInfo : variables)
			if (variableInfo.name == name)
				return &variableInfo;
		return nullptr;
	}

	Variable* Struct::getVariable(std::string_view name)
	{
		for (auto& variableInfo : variables)
			if (variableInfo.name == name)
				return &variableInfo;
		return nullptr;
	}

	bool CInterface::hasBase(std::string_view name)
	{
		for (auto& base : bases)
			if (base == name)
				return true;
		return false;
	}

	CMethod* CInterface::getCMethod(std::string_view name)
	{
		for (auto& cMethod : cMethods)
			if (cMethod.name == name)
				return &cMethod;
		return nullptr;
	}

	Method* CInterface::getMethod(std::string_view name)
	{
		for (auto& method : methods)
			if (method.name == name)
				return &method;
		return nullptr;
	}

	Enum* Header::getEnum(std::string_view name)
	{
		for (auto& enumInfo : enums)
			if (enumInfo.name == name)
				return &enumInfo;
		return nullptr;
	}

	Flags* Header::getFlags(std::string_view name)
	{
		for (auto& flagsInfo : flags)
			if (flagsInfo.name == name)
				return &flagsInfo;
		return nullptr;
	}

	Constant* Header::getConstant(std::string_view name)
	{
		for (auto& constant : constants)
			if (constant.name == name)
				return &constant;
		return nullptr;
	}

	Union* Header::getUnion(std::string_view name)
	{
		for (auto& unionInfo : unions)
			if (unionInfo.name == name)
				return &unionInfo;
		return nullptr;
	}

	Struct* Header::getStruct(std::string_view name)
	{
		for (auto& structInfo : structs)
			if (structInfo.name == name)
				return &structInfo;
		return nullptr;
	}

	GUID* Header::getGUID(std::string_view name)
	{
		for (auto& guidInfo : guids)
			if (guidInfo.name == name)
				return &guidInfo;
		return nullptr;
	}

	CInterface* Header::getCInterface(std::string_view name)
	{
		for (auto& cInterfaceInfo : cInterfaces)
			if (cInterfaceInfo.name == name)
				return &cInterfaceInfo;
		return nullptr;
	}

	CFunction* Header::getCFunction(std::string_view name)
	{
		for (auto& cFunctionInfo : cFunctions)
			if (cFunctionInfo.name == name)
				return &cFunctionInfo;
		return nullptr;
	}

	Function* Header::getFunction(std::string_view name)
	{
		for (auto& functionInfo : functions)
			if (functionInfo.name == name)
				return &functionInfo;
		return nullptr;
	}

	Header* SDK::getHeader(std::string_view name)
	{
		for (auto& header : headers)
			if (header.name == name)
				return &header;
		return nullptr;
	}


	void RemovePreviousEnum(Enum& newer, Enum& older)
	{
		for (auto ordinal = newer.ordinals.begin(); ordinal != newer.ordinals.end();)
		{
			auto previousOrdinal = older.getOrdinal(ordinal->name);
			if (!previousOrdinal)
			{
				// New ordinal
				++ordinal;
				continue;
			}

			if (previousOrdinal->uint64() == ordinal->uint64())
			{
				// Same ordinal
				ordinal = newer.ordinals.erase(ordinal);
				continue;
			}

			// Ordinal altered
			ordinal->altered = true;
			newer.altered    = true;
			++ordinal;
		}
	}

	void RemovePreviousFlags(Flags& newer, Flags& older)
	{
		for (auto flag = newer.flags.begin(); flag != newer.flags.end();)
		{
			auto previousFlag = older.getFlag(flag->name);
			if (!previousFlag)
			{
				// New flag
				++flag;
				continue;
			}

			if (previousFlag->uint64() == flag->uint64())
			{
				// Same flag
				flag = newer.flags.erase(flag);
				continue;
			}

			flag->altered = true;
			newer.altered = true;
			++flag;
		}
	}

	void RemovePreviousUnion(Union& newer, Union& older)
	{
		for (auto variableInfo = newer.variables.begin(); variableInfo != newer.variables.end();)
		{
			auto previousVariable = older.getVariable(variableInfo->name);
			if (!previousVariable)
			{
				// New variable
				newer.altered = true;
				break;
			}

			if (previousVariable->type != variableInfo->type)
			{
				// Different type
				newer.altered = true;
				break;
			}

			++variableInfo;
		}
	}

	void RemovePreviousStruct(Struct& newer, Struct& older)
	{
		for (auto variableInfo = newer.variables.begin(); variableInfo != newer.variables.end();)
		{
			auto previousVariable = older.getVariable(variableInfo->name);
			if (!previousVariable)
			{
				// New variable
				newer.altered = true;
				break;
			}

			if (previousVariable->type != variableInfo->type)
			{
				// Different type
				newer.altered = true;
				break;
			}

			++variableInfo;
		}
	}

	void RemovePreviousCInterface(CInterface& newer, CInterface& older)
	{
		if (newer.uuid != older.uuid)
		{
			newer.altered = true;
			return;
		}

		for (auto base = newer.bases.begin(); base != newer.bases.end();)
		{
			if (!older.hasBase(*base))
			{
				newer.altered = true;
				return;
			}

			++base;
		}

		for (auto cMethodInfo = newer.cMethods.begin(); cMethodInfo != newer.cMethods.end();)
		{
			auto previousCMethod = older.getCMethod(cMethodInfo->name);
			if (!previousCMethod)
			{
				// New CMethod
				newer.altered = true;
				return;
			}

			if (cMethodInfo->args.size() != previousCMethod->args.size())
			{
				newer.altered = true;
				return;
			}

			for (std::size_t i = 0; i < cMethodInfo->args.size(); ++i)
			{
				auto& argInfo         = cMethodInfo->args[i];
				auto& previousArgInfo = previousCMethod->args[i];
				if (argInfo.name != previousArgInfo.name ||
				    argInfo.type != previousArgInfo.type ||
				    argInfo.annotations != previousArgInfo.annotations)
				{
					newer.altered = true;
					return;
				}
			}

			if (cMethodInfo->returnType != previousCMethod->returnType ||
			    cMethodInfo->impl.has != previousCMethod->impl.has)
			{
				newer.altered = true;
				return;
			}

			if (cMethodInfo->impl.has)
			{
				if (cMethodInfo->impl.lang != previousCMethod->impl.lang ||
				    cMethodInfo->impl.code != previousCMethod->impl.code)
				{
					newer.altered = true;
				}
			}

			++cMethodInfo;
		}

		for (auto methodInfo = newer.methods.begin(); methodInfo != newer.methods.end();)
		{
			auto previousMethod = older.getMethod(methodInfo->name);
			if (!previousMethod)
			{
				// New Method
				newer.altered = true;
				return;
			}

			if (methodInfo->templateArgs.size() != previousMethod->templateArgs.size())
			{
				newer.altered = true;
				return;
			}

			for (std::size_t i = 0; i < methodInfo->templateArgs.size(); ++i)
			{
				auto& templateArgInfo         = methodInfo->templateArgs[i];
				auto& previousTemplateArgInfo = previousMethod->templateArgs[i];
				if (templateArgInfo.name != previousTemplateArgInfo.name ||
				    templateArgInfo.type != previousTemplateArgInfo.type ||
				    templateArgInfo.concept_ != previousTemplateArgInfo.concept_)
				{
					newer.altered = true;
					return;
				}
			}

			if (methodInfo->args.size() != previousMethod->args.size())
			{
				newer.altered = true;
				return;
			}

			for (std::size_t i = 0; i < methodInfo->args.size(); ++i)
			{
				auto& argInfo         = methodInfo->args[i];
				auto& previousArgInfo = previousMethod->args[i];
				if (argInfo.name != previousArgInfo.name ||
				    argInfo.type != previousArgInfo.type ||
				    argInfo.annotations != previousArgInfo.annotations)
				{
					newer.altered = true;
					return;
				}
			}

			if (methodInfo->returnType != previousMethod->returnType ||
			    methodInfo->constexpr_ != previousMethod->constexpr_ ||
			    methodInfo->impl.has != previousMethod->impl.has)
			{
				newer.altered = true;
				return;
			}

			if (methodInfo->impl.has)
			{
				if (methodInfo->impl.lang != previousMethod->impl.lang ||
				    methodInfo->impl.code != previousMethod->impl.code)
				{
					newer.altered = true;
				}
			}

			++methodInfo;
		}
	}

	void RemovePreviousHeader(Header& newer, Header& older)
	{
		for (auto enumInfo = newer.enums.begin(); enumInfo != newer.enums.end();)
		{
			auto previousEnum = older.getEnum(enumInfo->name);
			if (!previousEnum)
			{
				// New enum
				++enumInfo;
				continue;
			}

			RemovePreviousEnum(*enumInfo, *previousEnum);

			if (!enumInfo->altered && enumInfo->ordinals.empty())
			{
				// Nothing new or altered
				enumInfo = newer.enums.erase(enumInfo);
				continue;
			}

			enumInfo->append = true;
			++enumInfo;
		}

		for (auto flagsInfo = newer.flags.begin(); flagsInfo != newer.flags.end();)
		{
			auto previousFlags = older.getFlags(flagsInfo->name);
			if (!previousFlags)
			{
				// New flags
				++flagsInfo;
				continue;
			}

			RemovePreviousFlags(*flagsInfo, *previousFlags);

			if (!flagsInfo->altered && flagsInfo->flags.empty())
			{
				// Nothing new or altered
				flagsInfo = newer.flags.erase(flagsInfo);
				continue;
			}

			flagsInfo->append = true;
			++flagsInfo;
		}

		for (auto constant = newer.constants.begin(); constant != newer.constants.end();)
		{
			auto previousConstant = older.getConstant(constant->name);
			if (!previousConstant)
			{
				// New constant
				++constant;
				continue;
			}

			if (previousConstant->value == constant->value && previousConstant->type == constant->type)
			{
				// Same constant
				constant = newer.constants.erase(constant);
				continue;
			}

			constant->altered = true;
			++constant;
		}

		for (auto unionInfo = newer.unions.begin(); unionInfo != newer.unions.end();)
		{
			auto previousStruct = older.getUnion(unionInfo->name);
			if (!previousStruct)
			{
				// New struct
				++unionInfo;
				continue;
			}

			RemovePreviousUnion(*unionInfo, *previousStruct);

			if (!unionInfo->altered)
			{
				// Nothing new or altered
				unionInfo = newer.unions.erase(unionInfo);
				continue;
			}

			++unionInfo;
		}

		for (auto structInfo = newer.structs.begin(); structInfo != newer.structs.end();)
		{
			auto previousStruct = older.getStruct(structInfo->name);
			if (!previousStruct)
			{
				// New struct
				++structInfo;
				continue;
			}

			RemovePreviousStruct(*structInfo, *previousStruct);

			if (!structInfo->altered)
			{
				// Nothing new or altered
				structInfo = newer.structs.erase(structInfo);
				continue;
			}

			++structInfo;
		}

		for (auto guidInfo = newer.guids.begin(); guidInfo != newer.guids.end();)
		{
			auto previousGUID = older.getGUID(guidInfo->name);
			if (!previousGUID)
			{
				// New GUID
				++previousGUID;
				continue;
			}

			if (guidInfo->uuid == previousGUID->uuid)
			{
				// Same GUID
				guidInfo = newer.guids.erase(guidInfo);
				continue;
			}

			guidInfo->altered = true;
			++guidInfo;
		}

		for (auto cInterfaceInfo = newer.cInterfaces.begin(); cInterfaceInfo != newer.cInterfaces.end();)
		{
			auto previousCInterface = older.getCInterface(cInterfaceInfo->name);
			if (!previousCInterface)
			{
				// New CInterface
				++cInterfaceInfo;
				continue;
			}

			RemovePreviousCInterface(*cInterfaceInfo, *previousCInterface);

			if (!cInterfaceInfo->altered)
			{
				// Same CInterface
				cInterfaceInfo = newer.cInterfaces.erase(cInterfaceInfo);
				continue;
			}

			++cInterfaceInfo;
		}

		for (auto cFunctionInfo = newer.cFunctions.begin(); cFunctionInfo != newer.cFunctions.end();)
		{
			auto previousCFunction = older.getCFunction(cFunctionInfo->name);
			if (!previousCFunction)
			{
				// New CFunction
				++cFunctionInfo;
				continue;
			}

			if (cFunctionInfo->args.size() != previousCFunction->args.size())
			{
				cFunctionInfo->altered = true;
			}
			else
			{
				for (std::size_t i = 0; i < cFunctionInfo->args.size(); ++i)
				{
					auto& argInfo         = cFunctionInfo->args[i];
					auto& previousArgInfo = previousCFunction->args[i];
					if (argInfo.name != previousArgInfo.name ||
					    argInfo.type != previousArgInfo.type ||
					    argInfo.annotations != previousArgInfo.annotations)
					{
						cFunctionInfo->altered = true;
						break;
					}
				}
			}

			if (cFunctionInfo->returnType != previousCFunction->returnType)
				cFunctionInfo->altered = true;

			if (!cFunctionInfo->altered)
			{
				// Nothing new or altered
				cFunctionInfo = newer.cFunctions.erase(cFunctionInfo);
				continue;
			}

			++cFunctionInfo;
		}

		for (auto functionInfo = newer.functions.begin(); functionInfo != newer.functions.end();)
		{
			auto previousFunction = older.getFunction(functionInfo->name);
			if (!previousFunction)
			{
				// New Function
				++functionInfo;
				continue;
			}

			if (functionInfo->templateArgs.size() != previousFunction->templateArgs.size())
			{
				functionInfo->altered = true;
			}
			else
			{
				for (std::size_t i = 0; i < functionInfo->templateArgs.size(); ++i)
				{
					auto& templateArgInfo         = functionInfo->templateArgs[i];
					auto& previousTemplateArgInfo = previousFunction->templateArgs[i];
					if (templateArgInfo.name != previousTemplateArgInfo.name ||
					    templateArgInfo.type != previousTemplateArgInfo.type ||
					    templateArgInfo.concept_ != previousTemplateArgInfo.concept_)
					{
						functionInfo->altered = true;
						break;
					}
				}
			}

			if (functionInfo->args.size() != previousFunction->args.size())
			{
				functionInfo->altered = true;
			}
			else
			{
				for (std::size_t i = 0; i < functionInfo->args.size(); ++i)
				{
					auto& argInfo         = functionInfo->args[i];
					auto& previousArgInfo = previousFunction->args[i];
					if (argInfo.name != previousArgInfo.name ||
					    argInfo.type != previousArgInfo.type ||
					    argInfo.annotations != previousArgInfo.annotations)
					{
						functionInfo->altered = true;
						break;
					}
				}
			}

			if (functionInfo->returnType != previousFunction->returnType ||
			    functionInfo->constexpr_ != previousFunction->constexpr_ ||
			    functionInfo->impl.has != previousFunction->impl.has)
			{
				functionInfo->altered = true;
				return;
			}

			if (functionInfo->impl.has)
			{
				if (functionInfo->impl.lang != previousFunction->impl.lang ||
				    functionInfo->impl.code != previousFunction->impl.code)
				{
					functionInfo->altered = true;
				}
			}

			if (!functionInfo->altered)
			{
				// Nothing new or altered
				functionInfo = newer.functions.erase(functionInfo);
				continue;
			}

			++functionInfo;
		}
	}

	void RemovePreviousSDK(SDK& newer, SDK& older)
	{
		for (auto& header : newer.headers)
		{
			auto previousHeader = older.getHeader(header.name);
			if (!previousHeader)
			{
				// New header
				continue;
			}

			RemovePreviousHeader(header, *previousHeader);
		}
	}
} // namespace SDKInfo