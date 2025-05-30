//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

#include "abi_tag_on_virtual.hpp"
#include "header_exportable_declarations.hpp"
#include "hide_from_abi.hpp"
#include "internal_ftm_use.hpp"
#include "nodebug_on_aliases.hpp"
#include "proper_version_checks.hpp"
#include "robust_against_adl.hpp"
#include "robust_against_operator_ampersand.hpp"
#include "uglify_attributes.hpp"

namespace {
class LibcxxTestModule : public clang::tidy::ClangTidyModule {
public:
  void addCheckFactories(clang::tidy::ClangTidyCheckFactories& check_factories) override {
    check_factories.registerCheck<libcpp::abi_tag_on_virtual>("libcpp-avoid-abi-tag-on-virtual");
    check_factories.registerCheck<libcpp::header_exportable_declarations>("libcpp-header-exportable-declarations");
    check_factories.registerCheck<libcpp::hide_from_abi>("libcpp-hide-from-abi");
    check_factories.registerCheck<libcpp::internal_ftm_use>("libcpp-internal-ftms");
    check_factories.registerCheck<libcpp::nodebug_on_aliases>("libcpp-nodebug-on-aliases");
    check_factories.registerCheck<libcpp::proper_version_checks>("libcpp-cpp-version-check");
    check_factories.registerCheck<libcpp::robust_against_adl_check>("libcpp-robust-against-adl");
    check_factories.registerCheck<libcpp::robust_against_operator_ampersand>(
        "libcpp-robust-against-operator-ampersand");
    check_factories.registerCheck<libcpp::uglify_attributes>("libcpp-uglify-attributes");
  }
};
} // namespace

clang::tidy::ClangTidyModuleRegistry::Add<LibcxxTestModule> libcpp_module{
    "libcpp-module", "Adds libc++-specific checks."};
