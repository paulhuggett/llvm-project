//===--- mulsi3.c - Integer multiplication routines -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

int __mulsi3(int x, int y);
int __mulsi3(int x, int y) {
  unsigned xu = (unsigned)x;
  unsigned yu = (unsigned)y;
  unsigned result = 0;
  while (yu != 0) {
    if (yu & 1) {
      result += xu;
    }
    yu >>= 1;
    xu <<= 1;
  }
  return (int)result;
}
