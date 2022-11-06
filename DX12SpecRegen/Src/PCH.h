#pragma once

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/CommandLine.h>

#include <csignal>

#include <fstream>
#include <string>
#include <vector>
