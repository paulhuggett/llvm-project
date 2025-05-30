//===-- lib/Semantics/check-coarray.h ---------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_SEMANTICS_CHECK_COARRAY_H_
#define FORTRAN_SEMANTICS_CHECK_COARRAY_H_

#include "flang/Semantics/semantics.h"
#include <list>

namespace Fortran::semantics {

class CoarrayChecker : public virtual BaseChecker {
public:
  CoarrayChecker(SemanticsContext &context) : context_{context} {}
  void Leave(const parser::ChangeTeamStmt &);
  void Leave(const parser::EndChangeTeamStmt &);
  void Leave(const parser::SyncAllStmt &);
  void Leave(const parser::SyncImagesStmt &);
  void Leave(const parser::SyncMemoryStmt &);
  void Leave(const parser::SyncTeamStmt &);
  void Leave(const parser::NotifyWaitStmt &);
  void Leave(const parser::EventPostStmt &);
  void Leave(const parser::EventWaitStmt &);
  void Leave(const parser::LockStmt &);
  void Leave(const parser::UnlockStmt &);
  void Leave(const parser::CriticalStmt &);
  void Leave(const parser::ImageSelector &);
  void Leave(const parser::FormTeamStmt &);

  void Enter(const parser::CriticalConstruct &);
  void Enter(const parser::ChangeTeamConstruct &);

private:
  SemanticsContext &context_;

  void CheckNamesAreDistinct(const std::list<parser::CoarrayAssociation> &);
  void Say2(const parser::CharBlock &, parser::MessageFixedText &&,
      const parser::CharBlock &, parser::MessageFixedText &&);
};
} // namespace Fortran::semantics
#endif // FORTRAN_SEMANTICS_CHECK_COARRAY_H_
