#include "HeaderIntrospectionStage.h"
#include "Utils/Exception.h"
#include "Utils/Log.h"

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/CommandLine.h>

#include <fstream>
#include <string>

namespace ClangIntrospection
{
	void                         IntrospectMacro(const std::vector<std::string>& args, SDKInfo::Header* headerInfo, clang::MacroInfo* macro, std::string replacementString, std::string name, bool hasUnresolvedIdentifiers);
	std::string                  GetMacroReplacementStringAndRequirements(clang::Preprocessor& pp, clang::SourceManager& sourceManager, clang::MacroInfo* macro, std::vector<std::string>& args, bool& hasUnresolvedIdentifiers);
	const clang::IdentifierInfo* FindMacroDefinition(clang::Preprocessor& pp, std::string_view macroName);

	class HeaderIntrospectionASTVisitor : public clang::RecursiveASTVisitor<HeaderIntrospectionASTVisitor>
	{
	public:
		HeaderIntrospectionASTVisitor(clang::FileID mainFileID, clang::SourceManager* sourceManager, SDKInfo::Header* headerInfo)
		    : m_MainFileID(mainFileID), m_SourceManager(sourceManager), m_HeaderInfo(headerInfo) {}

		bool VisitEnumDecl(clang::EnumDecl* declaration)
		{
			if (!declInMainFile(declaration))
				return true;

			SDKInfo::Enum enumInfo {};
			enumInfo.name = declaration->getName();
			Assert(!enumInfo.name.empty(), "Enum name is empty!");
			auto type    = declaration->getPromotionType();
			auto typePtr = type.getTypePtr();
			if (typePtr->isBuiltinType())
			{
				auto bt = static_cast<const clang::BuiltinType*>(typePtr);
				switch (bt->getKind())
				{
				case clang::BuiltinType::Kind::Bool:
					enumInfo.type = Serialization::EIntegerType::Bool;
					break;
				case clang::BuiltinType::Kind::Char_U:
				case clang::BuiltinType::Kind::UChar:
					enumInfo.type = Serialization::EIntegerType::UInt8;
					break;
				case clang::BuiltinType::Kind::Char8:
					enumInfo.type = Serialization::EIntegerType::Char8;
					break;
				case clang::BuiltinType::Kind::WChar_U:
				case clang::BuiltinType::Kind::Char16:
					enumInfo.type = Serialization::EIntegerType::Char16;
					break;
				case clang::BuiltinType::Kind::UShort:
					enumInfo.type = Serialization::EIntegerType::UInt16;
					break;
				case clang::BuiltinType::Kind::Char32:
					enumInfo.type = Serialization::EIntegerType::Char32;
					break;
				case clang::BuiltinType::Kind::UInt:
					enumInfo.type = Serialization::EIntegerType::UInt32;
					break;
				case clang::BuiltinType::Kind::ULong:
				case clang::BuiltinType::Kind::ULongLong:
					enumInfo.type = Serialization::EIntegerType::UInt64;
					break;
				case clang::BuiltinType::Kind::UInt128:
					enumInfo.type = Serialization::EIntegerType::UInt128;
					break;
				case clang::BuiltinType::Kind::Char_S:
				case clang::BuiltinType::Kind::SChar:
					enumInfo.type = Serialization::EIntegerType::Int8;
					break;
				case clang::BuiltinType::Kind::WChar_S:
					enumInfo.type = Serialization::EIntegerType::Char16;
					break;
				case clang::BuiltinType::Kind::Short:
					enumInfo.type = Serialization::EIntegerType::Int16;
					break;
				case clang::BuiltinType::Kind::Int:
					enumInfo.type = Serialization::EIntegerType::Int32;
					break;
				case clang::BuiltinType::Kind::Long:
				case clang::BuiltinType::Kind::LongLong:
					enumInfo.type = Serialization::EIntegerType::Int64;
					break;
				case clang::BuiltinType::Kind::Int128:
					enumInfo.type = Serialization::EIntegerType::Int128;
					break;
				default:
					Assert(false, "Invalid enum type!");
					break;
				}
			}
			else
			{
				Assert(false, "Invalid enum type!");
			}

			for (auto enumerator : declaration->enumerators())
			{
				auto& ordinal = enumInfo.ordinals.emplace_back();
				ordinal.name  = enumerator->getName();
				Assert(!ordinal.name.empty(), "Ordinal name is empty!");
				auto& initVal = enumerator->getInitVal();
				ordinal.type  = enumInfo.type;
				ordinal.value = initVal.getLimitedValue(); // TODO(MarcasRealAccount): Support 128 bit integer values...
			}
			if (isEnumAFlags(enumInfo))
			{
				SDKInfo::Flags flags {};
				flags.name = std::move(enumInfo.name);
				flags.type = enumInfo.type;
				flags.flags.reserve(enumInfo.ordinals.size());
				for (auto& ordinal : enumInfo.ordinals)
				{
					auto& flag = flags.flags.emplace_back();
					flag.name  = std::move(ordinal.name);
					flag.type  = ordinal.type;
					flag.value = ordinal.value;
				}
				m_HeaderInfo->flags.emplace_back(std::move(flags));
			}
			else
			{
				m_HeaderInfo->enums.emplace_back(std::move(enumInfo));
			}

			return true;
		}

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* declaration)
		{
			if (!declInMainFile(declaration))
				return true;

			do {
				if (!declaration->isCompleteDefinition())
					break;

				if (declaration->isAnonymousStructOrUnion())
					break;

				if (declaration->getName().empty())
					break;

				if (declaration->isStruct())
					visitStruct(declaration);
				else if (declaration->isUnion())
					visitUnion(declaration);
			} while (false);

			return true;
		}

		bool VisitFunctionDecl(clang::FunctionDecl* declaration)
		{
			if (!declInMainFile(declaration))
				return true;

			do {
				if (!declaration->isGlobal())
					break;

				if (declaration->isOverloadedOperator())
					break;

				SDKInfo::CFunction cFunction {};
				cFunction.name = declaration->getName();
				Assert(!cFunction.name.empty(), "CFunction name is empty!");
				cFunction.returnType = declaration->getReturnType().getAsString();
				Assert(!cFunction.returnType.empty(), "CFunction return type is empty!");
				for (auto param : declaration->parameters())
				{
					auto& arg = cFunction.args.emplace_back();
					arg.name  = param->getName();
					Assert(!arg.name.empty(), "Arg name is empty!");
					arg.type = param->getType().getAsString();
					Assert(!arg.type.empty(), "Arg type is empty!");
				}

				m_HeaderInfo->cFunctions.emplace_back(std::move(cFunction));
			} while (false);

			return true;
		}

	private:
		void visitStruct(clang::CXXRecordDecl* declaration, std::string_view overrideName = "")
		{
			auto uuidAttribute = declaration->getAttr<clang::UuidAttr>();
			if (uuidAttribute)
				return visitInterface(declaration);

			SDKInfo::Struct structInfo {};
			if (overrideName.empty())
				structInfo.name = declaration->getName();
			else
				structInfo.name = overrideName;
			std::size_t i = 0;
			for (auto field : declaration->fields())
			{
				SDKInfo::Variable variable {};
				variable.name = field->getName();
				auto type     = field->getType();
				if (type->isRecordType())
				{
					auto rt = type->getAsCXXRecordDecl();

					if (rt->isAnonymousStructOrUnion() || rt->getName().empty())
					{
						std::string anonymousName = fmt::format("{}#{}", structInfo.name, i);
						if (rt->isStruct())
						{
							visitStruct(rt, anonymousName);
							variable.type = std::move(anonymousName);
							structInfo.variables.emplace_back(std::move(variable));
							++i;
						}
						else if (rt->isUnion())
						{
							visitUnion(rt, anonymousName);
							variable.type = std::move(anonymousName);
							structInfo.variables.emplace_back(std::move(variable));
							++i;
						}
					}
					else
					{
						variable.type = type.getAsString();
						structInfo.variables.emplace_back(std::move(variable));
					}
				}
				else
				{
					variable.type = type.getAsString();
					structInfo.variables.emplace_back(std::move(variable));
				}
			}
			m_HeaderInfo->structs.emplace_back(std::move(structInfo));
		}

		void visitUnion(clang::CXXRecordDecl* declaration, std::string_view overrideName = "")
		{
			SDKInfo::Union unionInfo {};
			if (overrideName.empty())
				unionInfo.name = declaration->getName();
			else
				unionInfo.name = overrideName;
			Assert(!unionInfo.name.empty(), "union name is empty!");
			std::size_t i = 0;
			for (auto field : declaration->fields())
			{
				SDKInfo::Variable variable {};
				variable.name = field->getName();
				auto type     = field->getType();
				if (type->isRecordType())
				{
					auto rt = type->getAsCXXRecordDecl();
					if (rt->isAnonymousStructOrUnion() || rt->getName().empty())
					{
						std::string anonymousName = fmt::format("{}#{}", unionInfo.name, i);
						if (rt->isStruct())
						{
							visitStruct(rt, anonymousName);
							variable.type = std::move(anonymousName);
							unionInfo.variables.emplace_back(std::move(variable));
							++i;
						}
						else if (rt->isUnion())
						{
							visitUnion(rt, anonymousName);
							variable.type = std::move(anonymousName);
							unionInfo.variables.emplace_back(std::move(variable));
							++i;
						}
					}
					else
					{
						variable.type = type.getAsString();
						unionInfo.variables.emplace_back(std::move(variable));
					}
				}
				else
				{
					variable.type = type.getAsString();
					unionInfo.variables.emplace_back(std::move(variable));
				}
			}
			m_HeaderInfo->unions.emplace_back(std::move(unionInfo));
		}

		void visitInterface(clang::CXXRecordDecl* declaration)
		{
			SDKInfo::CInterface cInterface {};
			cInterface.name = declaration->getName();
			Assert(!cInterface.name.empty(), "CInterface name is empty!");
			for (auto& base : declaration->bases())
				cInterface.bases.emplace_back(base.getType().getAsString());
			auto uuidAttribute = declaration->getAttr<clang::UuidAttr>();
			cInterface.uuid    = uuidAttribute->getGuid().lower();

			for (auto method : declaration->methods())
			{
				if (method->isImplicit())
					continue;

				auto& cMethod = cInterface.cMethods.emplace_back();
				cMethod.name  = method->getName();
				Assert(!cMethod.name.empty(), "CMethod name is empty!");
				cMethod.returnType = method->getReturnType().getAsString();
				Assert(!cMethod.returnType.empty(), "CMethod return type is empty!");
				for (auto param : method->parameters())
				{
					auto& arg = cMethod.args.emplace_back();
					arg.name  = param->getName();
					Assert(!arg.name.empty(), "Arg name is empty!");
					arg.type = param->getType().getAsString();
					Assert(!arg.type.empty(), "Arg type is empty!");
				}
			}
			m_HeaderInfo->cInterfaces.emplace_back(std::move(cInterface));
		}

		bool declInMainFile(clang::Decl* decl)
		{
			return m_SourceManager->getFileID(m_SourceManager->getFileLoc(decl->getBeginLoc())) == m_MainFileID;
		}

		bool isEnumAFlags(const SDKInfo::Enum& enumInfo)
		{
			bool hasFlagsInName = false;
			for (std::size_t i = 0, j = 0; !hasFlagsInName && i < enumInfo.name.size(); ++i)
			{
				switch (j)
				{
				case 0:
					if (enumInfo.name[i] == 'f' || enumInfo.name[i] == 'F')
						++j;
					else
						j = 0;
					break;
				case 1:
					if (enumInfo.name[i] == 'l' || enumInfo.name[i] == 'L')
						++j;
					else
						j = 0;
					break;
				case 2:
					if (enumInfo.name[i] == 'a' || enumInfo.name[i] == 'A')
						++j;
					else
						j = 0;
					break;
				case 3:
					if (enumInfo.name[i] == 'g' || enumInfo.name[i] == 'G')
					{
						hasFlagsInName = true;
						break;
					}
					else
					{
						j = 0;
					}
					break;
				}
			}
			if (hasFlagsInName)
				return true;

			std::size_t singleBitOrdinals = 0;
			for (auto& ordinal : enumInfo.ordinals)
				if (std::has_single_bit(ordinal.value))
					++singleBitOrdinals;

			return singleBitOrdinals > enumInfo.ordinals.size() * 4 / 5;
		}

	private:
		clang::FileID         m_MainFileID;
		clang::SourceManager* m_SourceManager;
		SDKInfo::Header*      m_HeaderInfo;
	};

	class HeaderIntrospectionASTConsumer : public clang::ASTConsumer
	{
	public:
		HeaderIntrospectionASTConsumer(clang::FileID mainFileID, clang::SourceManager* sourceManager, SDKInfo::Header* headerInfo)
		    : m_Visitor(mainFileID, sourceManager, headerInfo) {}

		virtual void HandleTranslationUnit(clang::ASTContext& context) override
		{
			m_Visitor.TraverseDecl(context.getTranslationUnitDecl());
		}

	private:
		HeaderIntrospectionASTVisitor m_Visitor;
	};

	class HeaderIntrospectionAction : public clang::ASTFrontendAction
	{
	public:
		HeaderIntrospectionAction(SDKInfo::Header* headerInfo)
		    : m_HeaderInfo(headerInfo) {}

		virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef inFile) override
		{
			auto& diagnostics = compiler.getDiagnostics();
			diagnostics.setClient(new clang::IgnoringDiagConsumer());

			return std::make_unique<HeaderIntrospectionASTConsumer>(compiler.getSourceManager().getMainFileID(), &compiler.getSourceManager(), m_HeaderInfo);
		}

		virtual void EndSourceFileAction() override
		{
			auto&         compiler      = getCompilerInstance();
			auto&         preprocessor  = compiler.getPreprocessor();
			auto&         sourceManager = compiler.getSourceManager();
			clang::FileID mainFileID    = sourceManager.getMainFileID();

			for (auto& macro : preprocessor.macros(false))
			{
				if (!macro.first->hasMacroDefinition())
					continue;

				auto definition = preprocessor.getMacroDefinition(macro.first);
				if (!definition)
					continue;

				auto macroInfo = definition.getMacroInfo();
				if (!macroInfo)
					continue;

				if (macroInfo->getNumTokens() == 0)
					continue;

				if (sourceManager.getFileID(macroInfo->getDefinitionLoc()) != mainFileID)
					continue;

				auto name = macro.first->getName();
				Assert(!name.empty(), "Macro name is empty!");
				if (name.startswith("__"))
					continue;

				std::vector<std::string> args;
				args.reserve(128);
				args.emplace_back("-std=c++20");

				bool        unresolvedIdentifiers = false;
				std::string replacementString     = GetMacroReplacementStringAndRequirements(preprocessor, sourceManager, macroInfo, args, unresolvedIdentifiers);

				IntrospectMacro(args, m_HeaderInfo, macroInfo, std::move(replacementString), name.data(), unresolvedIdentifiers);
			}
		}

	private:
		SDKInfo::Header* m_HeaderInfo;
	};

	bool Introspect(const std::vector<std::string>& args, std::filesystem::path path, SDKInfo::Header* headerInfo)
	{
		std::string filename = path.filename().replace_extension(".cpp").string();

		std::string   code;
		std::ifstream file { path, std::ios::ate };
		if (!file)
		{
			Log::Critical("Failed to open '{}'", path.string());
			return false;
		}
		code.resize(file.tellg());
		file.seekg(0);
		file.read(code.data(), code.size());
		file.close();
		code.resize(code.find_last_not_of('\0'));

		return clang::tooling::runToolOnCodeWithArgs(std::make_unique<HeaderIntrospectionAction>(headerInfo), code, args, filename, "clang-tool", std::make_shared<clang::PCHContainerOperations>());
	}

	void IntrospectMacro(const std::vector<std::string>& args, SDKInfo::Header* headerInfo, clang::MacroInfo* macro, std::string replacementString, std::string name, bool hasUnresolvedIdentifiers)
	{
		if (macro->isFunctionLike())
		{
			auto& function      = headerInfo->functions.emplace_back();
			function.name       = std::move(name);
			function.returnType = "auto";
			function.args.reserve(macro->getNumParams());
			for (auto& param : macro->params())
			{
				auto& arg = function.args.emplace_back();
				arg.name  = param->getName();
				arg.type  = "auto";
				Assert(!arg.name.empty(), "Macro parameter name is empty!");
			}
			function.constexpr_ = true;

			function.impl.has  = true;
			function.impl.lang = "C++";
			function.impl.code = fmt::format("return {};", replacementString);
		}
		else
		{
			auto& constant = headerInfo->constants.emplace_back();
			constant.name  = std::move(name);
			std::unique_ptr<clang::ASTUnit> constantAST;
			clang::TranslationUnitDecl*     TU = nullptr;
			if (!hasUnresolvedIdentifiers)
			{
				constantAST = clang::tooling::buildASTFromCodeWithArgs(fmt::format("constexpr auto x = {};", replacementString), args);
				TU          = constantAST->getASTContext().getTranslationUnitDecl();
			}
			bool assignedConstant = false;
			if (TU)
			{
				for (auto decl : TU->decls())
				{
					if (decl->getKind() == clang::Decl::Kind::Var)
					{
						auto varDecl = static_cast<clang::VarDecl*>(decl);
						auto value   = varDecl->evaluateValue();
						switch (value->getKind())
						{
						case clang::APValue::Int:
						{
							auto& intValue = value->getInt();
							if (intValue.isSigned())
							{
								auto bitWidth = intValue.getBitWidth();
								constant.type = fmt::format("i{}", bitWidth);
							}
							else
							{
								auto bitWidth = intValue.getBitWidth();
								constant.type = fmt::format("u{}", bitWidth);
							}
							llvm::SmallString<1024> str;
							intValue.toString(str);
							constant.value   = std::string_view { str.data(), str.size() };
							assignedConstant = true;
							break;
						}
						case clang::APValue::Float:
						{
							auto& floatValue = value->getFloat();
							switch (llvm::APFloatBase::SemanticsToEnum(floatValue.getSemantics()))
							{
							case llvm::APFloatBase::Semantics::S_IEEEhalf:
								constant.type = "f16";
								break;
							case llvm::APFloatBase::Semantics::S_BFloat:
								constant.type = "f16";
								break;
							case llvm::APFloatBase::Semantics::S_IEEEsingle:
								constant.type = "f32";
								break;
							case llvm::APFloatBase::Semantics::S_IEEEdouble:
								constant.type = "f64";
								break;
							case llvm::APFloatBase::Semantics::S_IEEEquad:
								constant.type = "f128";
								break;
							case llvm::APFloatBase::Semantics::S_PPCDoubleDouble:
								constant.type = "f128";
								break;
							case llvm::APFloatBase::Semantics::S_Float8E5M2:
								constant.type = "f8";
								break;
							case llvm::APFloatBase::Semantics::S_x87DoubleExtended:
								constant.type = "f80";
								break;
							default:
								Log::Critical("WTF unknown float type detected for macro '{}' in header '{}'!!!", constant.name, headerInfo->name);
								break;
							}
							llvm::SmallString<1024> str;
							floatValue.toString(str);
							constant.value   = std::string_view { str.data(), str.size() };
							assignedConstant = true;
							break;
						}
						case clang::APValue::FixedPoint:
						{
							auto& fixedValue = value->getFixedPoint();
							if (fixedValue.isSigned())
							{
								auto width      = fixedValue.getWidth();
								auto fractional = fixedValue.getScale();
								constant.type   = fmt::format("i{}.{}", width - fractional, fractional);
							}
							else
							{
								auto width      = fixedValue.getWidth();
								auto fractional = fixedValue.getScale();
								constant.type   = fmt::format("u{}.{}", width - fractional, fractional);
							}

							llvm::SmallString<1024> str;
							fixedValue.toString(str);
							constant.value   = std::string_view { str.data(), str.size() };
							assignedConstant = true;
							break;
						}
						case clang::APValue::ComplexInt:
							Log::Error("ComplexInt not implemented");
							break;
						case clang::APValue::ComplexFloat:
							Log::Error("ComplexFloat not implemented");
							break;
						case clang::APValue::LValue:
							Log::Error("LValue not implemented");
							break;
						case clang::APValue::Vector:
							Log::Error("Vector not implemented");
							break;
						case clang::APValue::Array:
							Log::Error("Array not implemented");
							break;
						case clang::APValue::Struct:
							Log::Error("Struct not implemented");
							break;
						case clang::APValue::Union:
							Log::Error("Union not implemented");
							break;
						case clang::APValue::MemberPointer:
							Log::Error("MemberPointer not implemented");
							break;
						case clang::APValue::AddrLabelDiff:
							Log::Error("AddrLabelDiff not implemented");
							break;
						default: break;
						}
						break;
					}
				}
			}

			if (!assignedConstant)
			{
				constant.type  = "auto";
				constant.value = std::move(replacementString);
			}
		}
	}

	std::string GetMacroReplacementStringAndRequirements(clang::Preprocessor& pp, clang::SourceManager& sourceManager, clang::MacroInfo* macro, std::vector<std::string>& args, bool& hasUnresolvedIdentifiers)
	{
		std::string replacementString;
		for (auto& token : macro->tokens())
		{
			auto start = sourceManager.getCharacterData(token.getLocation());
			auto end   = sourceManager.getCharacterData(token.getEndLoc());

			std::string_view str = std::string_view { start, end };
			replacementString += str;

			if (token.getKind() == clang::tok::TokenKind::identifier)
			{
				for (auto& param : macro->params())
					if (param->getName().data() == str)
						continue;

				auto requiredMacro = FindMacroDefinition(pp, str);
				if (!requiredMacro)
				{
					hasUnresolvedIdentifiers = hasUnresolvedIdentifiers || true;
					continue;
				}

				if (!requiredMacro->hasMacroDefinition())
				{
					args.emplace_back("-D");
					args.emplace_back(str);
					continue;
				}

				auto definition = pp.getMacroDefinition(requiredMacro);
				if (!definition)
					continue;

				auto macroInfo = definition.getMacroInfo();
				if (!macroInfo)
					continue;

				if (macroInfo->isBuiltinMacro())
					continue;

				if (macroInfo->getNumTokens() == 0)
					continue;

				std::string replacementStr = GetMacroReplacementStringAndRequirements(pp, sourceManager, macroInfo, args, hasUnresolvedIdentifiers);
				args.emplace_back("-D");
				if (macroInfo->isFunctionLike())
				{
					std::string params;
					for (auto param : macroInfo->params())
					{
						if (!params.empty())
							params += ',';
						params += param->getName();
					}
					args.emplace_back(fmt::format("{}({})={}", str, params, replacementStr));
				}
				else
				{
					args.emplace_back(fmt::format("{}={}", str, replacementStr));
				}
			}
		}
		return replacementString;
	}

	const clang::IdentifierInfo* FindMacroDefinition(clang::Preprocessor& pp, std::string_view macroName)
	{
		for (auto& macro : pp.macros())
			if (macro.first->getName().data() == macroName)
				return macro.first;
		return nullptr;
	}
} // namespace ClangIntrospection