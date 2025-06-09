//===-- RISCVKeysomExpand.cpp - Expand pseudo instructions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/MCContext.h"

using namespace llvm;

#define RISCV_KEYSOM_EXPAND "RISC-V Keysom disabled instruction expansion pass"

namespace {

class InstructionHelper {
public:
  InstructionHelper(MachineRegisterInfo &MRI,
                    const TargetRegisterClass *const RegClass,
                    MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                    const DebugLoc &DL, const RISCVSubtarget *STI,
                    const RISCVInstrInfo *TII)
      : MRI_{MRI}, RegClass_{RegClass}, OrigBB_{OrigBB}, MBBI_{MBBI}, DL_{DL},
        STI_{STI}, TII_{TII} {}
  [[nodiscard]] Register rvAddi(Register Rs1, int64_t Immediate) {
    return this->buildImmediate(true, RISCV::ADDI, RISCV::ADDI, Rs1, Immediate);
  }
  void rvAddi(Register Rd, Register Rs1, int64_t Immediate) {
    this->buildImmediate(true, RISCV::ADDI, RISCV::ADDI, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvSub(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SUB, Rs1, Rs2);
  }
  void rvSub(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::SUB, Rd, Rs1, Rs2);
  }

  [[nodiscard]] Register rvXori(Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoXori(), RISCV::XORI,
                                RISCV::XOR, Rs1, Immediate);
  }
  void rvXori(Register Rd, Register Rs1, int64_t Immediate) {
    this->buildImmediate(!STI_->hasVendorXKeysomNoXori(), RISCV::XORI,
                         RISCV::XOR, Rd, Rs1, Immediate);
  }
  [[nodiscard]] Register rvAnd(Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoAndi(), RISCV::ANDI,
                                RISCV::AND, Rs1, Immediate);
  }
  [[nodiscard]] Register rvAnd(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::AND, Rs1, Rs2);
  }
  void rvAnd(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::AND, Rd, Rs1, Rs2);
  }

  [[nodiscard]] Register rvAndi(Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoAndi(), RISCV::ANDI,
                                RISCV::AND, Rs1, Immediate);
  }
  void rvAndi(Register Rd, Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoAndi(), RISCV::ANDI,
                                RISCV::AND, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvOri(Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoOri(), RISCV::ORI,
                                RISCV::OR, Rs1, Immediate);
  }
  void rvOri(Register Rd, Register Rs1, int64_t Immediate) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoOri(), RISCV::ORI,
                                RISCV::OR, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvSll(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SLL, Rs1, Rs2);
  }
  [[nodiscard]] Register rvSlli(Register Rs1, int64_t ShAmt) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSlli(), RISCV::SLLI,
                                RISCV::SLL, Rs1, ShAmt);
  }

  [[nodiscard]] Register rvSra(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SRA, Rs1, Rs2);
  }
  void rvSra(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::SRA, Rd, Rs1, Rs2);
  }

  [[nodiscard]] Register rvSrai(Register Rs1, int64_t ShAmt) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSrai(), RISCV::SRAI,
                                RISCV::SRA, Rs1, ShAmt);
  }
  void rvSrai(Register Rd, Register Rs1, int64_t ShAmt) {
    this->buildImmediate(!STI_->hasVendorXKeysomNoSrai(), RISCV::SRAI,
                         RISCV::SRA, Rd, Rs1, ShAmt);
  }

  void rvSlli(Register Rd, Register Rs1, int64_t ShAmt) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSlli(), RISCV::SLLI,
                                RISCV::SLL, Rd, Rs1, ShAmt);
  }

  [[nodiscard]] Register rvSrli(Register Rs1, int64_t ShAmt) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSrli(), RISCV::SRLI,
                                RISCV::SRL, Rs1, ShAmt);
  }
  void rvSrli(Register Rd, Register Rs1, int64_t ShAmt) {
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSrli(), RISCV::SRLI,
                                RISCV::SRL, Rd, Rs1, ShAmt);
  }

  [[nodiscard]] Register rvSrl(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SRL, Rs1, Rs2);
  }
  void rvSrl(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::SRL, Rd, Rs1, Rs2);
  }

private:
  [[nodiscard]] Register buildImmediate(bool HasInst, int ImmInstr,
                                        int RegInstr, Register Rs1,
                                        int64_t Immediate) {
    Register Rd = MRI_.createVirtualRegister(RegClass_);
    this->buildImmediate(HasInst, ImmInstr, RegInstr, Rd, Rs1, Immediate);
    return Rd;
  }
  void buildImmediate(bool HasInst, int ImmInstr, int RegInstr, Register Rd,
                      Register Rs1, int64_t Immediate) {
    if (HasInst) {
      BuildMI(OrigBB_, MBBI_, DL_, TII_->get(ImmInstr), Rd)
          .addReg(Rs1)
          .addImm(Immediate);
      return;
    }
    auto ImmReg = rvAddi(RISCV::X0, Immediate);
    this->buildTwoReg(RegInstr, Rd, Rs1, ImmReg);
  }

  [[nodiscard]] Register buildTwoReg(int Instr, Register Rs1, Register Rs2) {
    Register Rd = MRI_.createVirtualRegister(RegClass_);
    this->buildTwoReg(Instr, Rd, Rs1, Rs2);
    return Rd;
  }
  void buildTwoReg(int Instr, Register Rd, Register Rs1, Register Rs2) {
    BuildMI(OrigBB_, MBBI_, DL_, TII_->get(Instr), Rd).addReg(Rs1).addReg(Rs2);
  }

  MachineRegisterInfo &MRI_;
  const TargetRegisterClass *RegClass_;
  MachineBasicBlock &OrigBB_;
  MachineBasicBlock::iterator MBBI_;
  const DebugLoc &DL_;

  const RISCVSubtarget *STI_;
  const RISCVInstrInfo *TII_;
};

class RISCVKeysomExpand : public MachineFunctionPass {
public:
  static char ID;

  explicit RISCVKeysomExpand(bool IsPreRA)
      : MachineFunctionPass(ID), IsPreRA_{IsPreRA} {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineBranchProbabilityInfoWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  StringRef getPassName() const override { return RISCV_KEYSOM_EXPAND; }

private:
  const RISCVSubtarget *STI_ = nullptr;
  const RISCVInstrInfo *TII_ = nullptr;
  bool IsPreRA_;

  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);

  bool expandSLTIU(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   MachineBasicBlock::iterator &NextMBBI);
  bool expandSLT(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
  bool expandSLTU(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandSRLI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandSRL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
  bool expandSRAI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandOR(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandBEQ(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
  bool expandBNE(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
  bool expandBGE(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
  bool expandBGEU(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);

  bool expandBranchGreaterEqual(RISCVCC::CondCode CC, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI,
                                MachineBasicBlock::iterator &NextMBBI);
  bool expandSetLessThan(RISCVCC::CondCode CC, MachineBasicBlock &OrigBB,
                         MachineBasicBlock::iterator MBBI,
                         MachineBasicBlock::iterator &NextMBBI);
};
char RISCVKeysomExpand::ID = 0;

bool RISCVKeysomExpand::runOnMachineFunction(MachineFunction &MF) {
  STI_ = &MF.getSubtarget<RISCVSubtarget>();
  TII_ = STI_->getInstrInfo();

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);
  return Modified;
}

bool RISCVKeysomExpand::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }
  return Modified;
}

bool RISCVKeysomExpand::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {

  switch (MBBI->getOpcode()) {
  case RISCV::PseudoSLTIU:
    return expandSLTIU(MBB, MBBI, NextMBBI);
  case RISCV::SLT:
  case RISCV::PseudoSLT:
    return expandSLT(MBB, MBBI, NextMBBI);
  case RISCV::OR:
    return expandOR(MBB, MBBI, NextMBBI);
  case RISCV::BEQ:
    return expandBEQ(MBB, MBBI, NextMBBI);
  case RISCV::BNE:
    return expandBNE(MBB, MBBI, NextMBBI);
  case RISCV::BGE:
    return expandBGE(MBB, MBBI, NextMBBI);
  case RISCV::BGEU:
    return expandBGEU(MBB, MBBI, NextMBBI);
  case RISCV::PseudoSLTU:
    return expandSLTU(MBB, MBBI, NextMBBI);
  case RISCV::SRLI:
    return expandSRLI(MBB, MBBI, NextMBBI);
  case RISCV::SRL:
    return expandSRL(MBB, MBBI, NextMBBI);
  case RISCV::SRAI:
    return expandSRAI(MBB, MBBI, NextMBBI);
  }
  return false;
}

// PseudoSLTIU takes the same operands as the SLTIU instruction:
//
//   PseudoSLTIU rd, rs1, imm
//
bool RISCVKeysomExpand::expandSLTIU(MachineBasicBlock &OrigBB,
                                    MachineBasicBlock::iterator MBBI,
                                    MachineBasicBlock::iterator &NextMBBI) {
  if (!IsPreRA_) {
    assert(false && "Can't expand SLTIU after register allocation");
    return false;
  }
  if (!STI_->hasVendorXKeysomNoSltiu()) {
    if (MBBI->getOpcode() == RISCV::PseudoSLTIU) {
      // MachineInstr &MI = *MBBI;
      //  Simply swap PseudoSLTIU for real SLTIU.
      BuildMI(OrigBB, MBBI, MBBI->getDebugLoc(), TII_->get(RISCV::SLTIU),
              MBBI->getOperand(0).getReg())
          .addReg(MBBI->getOperand(1).getReg())
          .addReg(MBBI->getOperand(2).getReg());
      MBBI->eraseFromParent();
      return true;
    }
    return false;
  }

  static constexpr auto Zero = RISCV::X0;
  MachineFunction *const MF = OrigBB.getParent();
  assert(MF->getSubtarget<RISCVSubtarget>().hasVendorXKeysomNoSltiu() &&
         "PseudoSLTIU should only be used when SLTIU is disabled");
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected PseudoSLTIU to have 3 operands "
                                     "(matching the SLTIU instruction)");
  DebugLoc DL = MI.getDebugLoc();

  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  const int64_t Imm = MI.getOperand(2).getImm();
  MachineRegisterInfo &MRI = MF->getRegInfo();

  if (!STI_->hasVendorXKeysomNoSltu()) {
    // Use "sltu" if we have it. The replacement is straightforward:
    //
    //      [... previous instrs ...]
    //      addi  Rs2, zero, imm
    //      sltu  rd, rs1, Rs2
    //      [... later instrs ...]
    Register Rs2 = MRI.createVirtualRegister(MRI.getRegClass(Rd));
    BuildMI(OrigBB, MBBI, DL, TII_->get(RISCV::ADDI), Rs2)
        .addReg(Zero)
        .addImm(Imm);
    BuildMI(OrigBB, MBBI, DL, TII_->get(RISCV::SLTU), Rd)
        .addReg(Rs1)
        .addReg(Rs2);
    MI.eraseFromParent();
    return true;
  }

  // The replacement code should now look like:
  //
  //  OrigBB:
  //      [... previous instrs ...]
  //      addi  SubResult, rs1, -imm
  //      addi  ImmReg, zero, imm
  //      bgeu  SubResult, ImmReg, TrueBB
  //  FalseBB:
  //      addi  FalseReg, zero, 0
  //      jal   X0, PostBB
  //  TrueBB:
  //      addi  TrueReg, zero, 1
  //      ; Fallthrough
  //  PostBB:
  //      rd = PHI [TrueReg, TrueBB], [FalseReg, FalseBB]
  //      [... later instrs ...]

  MachineBasicBlock *const FalseBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
  MachineBasicBlock *const TrueBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
  MachineBasicBlock *const PostBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());

  MachineFunction::iterator It = ++OrigBB.getIterator();
  MF->insert(It, FalseBB);
  MF->insert(It, TrueBB);
  MF->insert(It, PostBB);

  // Transfer rest of current basic-block to PostBB
  PostBB->splice(PostBB->begin(), &OrigBB,
                 std::next(MachineBasicBlock::iterator{MI}), OrigBB.end());
  PostBB->transferSuccessorsAndUpdatePHIs(&OrigBB);

  Register SubResult = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(OrigBB, OrigBB.end(), DL, TII_->get(RISCV::ADDI), SubResult)
      .addReg(Rs1)
      .addImm(-Imm);
  Register ImmReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(OrigBB, OrigBB.end(), DL, TII_->get(RISCV::ADDI), ImmReg)
      .addReg(Zero)
      .addImm(Imm);
  BuildMI(OrigBB, OrigBB.end(), DL, TII_->get(RISCV::BGEU))
      .addReg(SubResult)
      .addReg(ImmReg)
      .addMBB(TrueBB);
  OrigBB.addSuccessor(TrueBB);
  OrigBB.addSuccessor(FalseBB);

  Register FalseReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*FalseBB, FalseBB->end(), DL, TII_->get(RISCV::ADDI), FalseReg)
      .addReg(Zero)
      .addImm(0);
  BuildMI(*FalseBB, FalseBB->end(), DL, TII_->get(RISCV::PseudoBR))
      .addMBB(PostBB);
  FalseBB->addSuccessor(PostBB);

  Register TrueReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*TrueBB, TrueBB->end(), DL, TII_->get(RISCV::ADDI), TrueReg)
      .addReg(Zero)
      .addImm(1);
  // TrueBB falls through.
  TrueBB->addSuccessor(PostBB);

  // A phi node to def the final result.
  BuildMI(*PostBB, PostBB->begin(), DL, TII_->get(TargetOpcode::PHI), Rd)
      .addReg(FalseReg)
      .addMBB(FalseBB)
      .addReg(TrueReg)
      .addMBB(TrueBB);

  NextMBBI = OrigBB.end();
  MI.eraseFromParent();

  return true;
}

bool RISCVKeysomExpand::expandSRLI(MachineBasicBlock &OrigBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *const MF = OrigBB.getParent();
  if (!STI_->hasVendorXKeysomNoSrli()) {
    return false;
  }

  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SRLI to have 3 operands");
  DebugLoc DL = MI.getDebugLoc();

  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  int64_t ShAmt = MI.getOperand(2).getImm();

  MachineRegisterInfo &MRI = MF->getRegInfo();
  InstructionHelper Helper{MRI, MRI.getRegClass(Rd), OrigBB, MBBI, DL, STI_,
                           TII_};

  if (!STI_->hasVendorXKeysomNoSrl()) {
    // The SRL instruction is available, so use it.
    Helper.rvSrli(Rd, Rs1, ShAmt);
    MI.eraseFromParent();
    return true;
  }

  // Use sra and a mask. Starting with the instruction:
  //
  //   rd = srli rs1, shamt
  //
  // The replacement looks like:
  //
  //   ShiftA = sra rs1, rs2
  //   SextMask = (1 << (XLen - shamt)) - 1
  //   rd = ShiftA & SextMask
  auto ShiftA = Helper.rvSrai(Rs1, ShAmt);
  // Now mask out the effect of the sign extension that SRA performs.
  const auto Mask = (1U << (STI_->getXLen() - ShAmt)) - 1U;
  if (Mask >= 1 << 12) {
    Register UpperImm = MRI.createVirtualRegister(MRI.getRegClass(Rd));
    // TODO: in theory, LUI is an instruction that can be disabled!
    BuildMI(OrigBB, MBBI, DL, TII_->get(RISCV::LUI), UpperImm)
        .addImm((Mask >> 12) + 1U);
    auto FullMask = Helper.rvAddi(UpperImm, -1);
    Helper.rvAnd(Rd, ShiftA, FullMask);
  } else {
    Helper.rvAndi(Rd, ShiftA, Mask);
  }
  MI.eraseFromParent();
  return true;
}

// Use to expand both SRLI and SRL
bool RISCVKeysomExpand::expandSRL(MachineBasicBlock &OrigBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *const MF = OrigBB.getParent();
  if (!STI_->hasVendorXKeysomNoSrl()) {
    return false;
  }

  static constexpr auto Zero = RISCV::X0;
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SRL to have 3 operands");
  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  Register Rs2 = MI.getOperand(2).getReg();

  MachineRegisterInfo &MRI = MF->getRegInfo();
  InstructionHelper Helper{
      MRI, MRI.getRegClass(Rd), OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};

  // Use sra and a mask. Starting with the instruction:
  //
  //   rd = srl rs1, rs2
  //
  // The replacement looks like:
  //
  //   ShiftA = sra rs1, rs2
  //   AllOnes = addi Zero, -1
  //   Rs2Bounded = and rs2, 0b11111   ; use rs2's least-significant 5 bits
  //   Dist = sub Xlen, Rs2Bounded     ; how far to shift AllOnes
  //   SextMaskInv = sll AllOnes, Dist ; create the inverted mask
  //   SextMask = xori SextMaskInv, -1 ; invert to get the true mask
  //   rd = ShiftA & SextMask

  auto ShiftA = Helper.rvSra(Rs1, Rs2);
  auto AllOnes = Helper.rvAddi(Zero, -1);
  // Now mask out the effect of the sign extension that SRA performs.
  // Zero all but the lower 5 bits of rs2
  auto Rs2Bounded = Helper.rvAndi(Rs2, 0b11111);
  // How far to shift AllOnes
  auto XLenImm = Helper.rvAddi(Zero, STI_->getXLen());
  auto Dist = Helper.rvSub(XLenImm, Rs2Bounded);
  // Create the inverted mask
  Register SextMaskInv = Helper.rvSll(AllOnes, Dist);
  // Invert to get the true mask
  Register SextMask = Helper.rvXori(SextMaskInv, -1);

  Helper.rvAnd(Rd, ShiftA, SextMask);

  MI.eraseFromParent();
  return true;
}

bool RISCVKeysomExpand::expandOR(MachineBasicBlock &OrigBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoOr()) {
    return false;
  }

  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected OR to have 3 operands");
  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  Register Rs2 = MI.getOperand(2).getReg();

  MachineRegisterInfo &MRI = OrigBB.getParent()->getRegInfo();
  InstructionHelper Helper{
      MRI, MRI.getRegClass(Rd), OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};
  Register N1 = Helper.rvXori(Rs1, -1);
  Register N2 = Helper.rvXori(Rs2, -1);
  Register A = Helper.rvAnd(N1, N2);
  Helper.rvXori(Rd, A, -1);
  MI.eraseFromParent();
  return true;
}

bool RISCVKeysomExpand::expandSLT(MachineBasicBlock &OrigBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoSlt()) {
    if (MBBI->getOpcode() == RISCV::PseudoSLT) {
      // MachineInstr &MI = *MBBI;
      //  Simply swap PseudoSLT for real SLT.
      BuildMI(OrigBB, MBBI, MBBI->getDebugLoc(), TII_->get(RISCV::SLT),
              MBBI->getOperand(0).getReg())
          .addReg(MBBI->getOperand(1).getReg())
          .addReg(MBBI->getOperand(2).getReg());
      MBBI->eraseFromParent();
      return true;
    }
    return false;
  }
  return this->expandSetLessThan(RISCVCC::COND_LT, OrigBB, MBBI, NextMBBI);
}
bool RISCVKeysomExpand::expandSLTU(MachineBasicBlock &OrigBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoSltu()) {
    if (MBBI->getOpcode() == RISCV::PseudoSLTU) {
      // MachineInstr &MI = *MBBI;
      //  Simply swap PseudoSLTU for real SLTU.
      BuildMI(OrigBB, MBBI, MBBI->getDebugLoc(), TII_->get(RISCV::SLTU),
              MBBI->getOperand(0).getReg())
          .addReg(MBBI->getOperand(1).getReg())
          .addReg(MBBI->getOperand(2).getReg());
      MBBI->eraseFromParent();
      return true;
    }
    return false;
  }
  return this->expandSetLessThan(RISCVCC::COND_LTU, OrigBB, MBBI, NextMBBI);
}

bool RISCVKeysomExpand::expandSetLessThan(
    RISCVCC::CondCode CC, MachineBasicBlock &OrigBB,
    MachineBasicBlock::iterator MBBI, MachineBasicBlock::iterator &NextMBBI) {

  if (!IsPreRA_) {
    assert(false && "Can't do expandSetLessThan() after register allocation");
    return false;
  }

  // The replacement code should look like:
  //
  //  OrigBB:
  //      [... previous instrs ...]
  //      BranchOpcode rs1, rs2, TrueBB
  //  FalseBB:
  //      addi  FalseReg, zero, 0
  //      jal   X0, PostBB
  //  TrueBB:
  //      addi  TrueReg, zero, 1
  //      ; Fallthrough
  //  PostBB:
  //      rd = PHI [TrueReg, TrueBB], [FalseReg, FalseBB]
  //      [... later instrs ...]

  static constexpr auto Zero = RISCV::X0;
  MachineFunction *const MF = OrigBB.getParent();
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected PseudoSLT[U] to have 3 operands "
                                     "(matching the SLT[U] instruction)");
  DebugLoc DL = MI.getDebugLoc();

  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  Register Rs2 = MI.getOperand(2).getReg();
  MachineRegisterInfo &MRI = MF->getRegInfo();

  MachineBasicBlock *const FalseBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
  MachineBasicBlock *const TrueBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
  MachineBasicBlock *const PostBB =
      MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());

  MachineFunction::iterator It = ++OrigBB.getIterator();
  MF->insert(It, FalseBB);
  MF->insert(It, TrueBB);
  MF->insert(It, PostBB);

  // Transfer the rest of the current basic-block to PostBB
  PostBB->splice(PostBB->begin(), &OrigBB,
                 std::next(MachineBasicBlock::iterator{MI}), OrigBB.end());
  PostBB->transferSuccessorsAndUpdatePHIs(&OrigBB);

  const MachineOperand Cond[] = {
      MachineOperand::CreateImm(CC),
      MachineOperand::CreateReg(Rs1, /*isDef=*/false),
      MachineOperand::CreateReg(Rs2, /*isDef=*/false),
  };
  TII_->insertBranch(OrigBB, TrueBB, FalseBB, Cond, DL);
  OrigBB.addSuccessor(FalseBB);
  OrigBB.addSuccessor(TrueBB);

  Register FalseReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*FalseBB, FalseBB->end(), DL, TII_->get(RISCV::ADDI), FalseReg)
      .addReg(Zero)
      .addImm(0);
  TII_->insertBranch(*FalseBB, PostBB, nullptr, {}, DL);
  FalseBB->addSuccessor(PostBB);

  Register TrueReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*TrueBB, TrueBB->end(), DL, TII_->get(RISCV::ADDI), TrueReg)
      .addReg(Zero)
      .addImm(1);
  TII_->insertBranch(*TrueBB, PostBB, nullptr, {}, DL);
  TrueBB->addSuccessor(PostBB);

  // A phi node to def the final result.
  BuildMI(*PostBB, PostBB->begin(), DL, TII_->get(TargetOpcode::PHI), Rd)
      .addReg(FalseReg)
      .addMBB(FalseBB)
      .addReg(TrueReg)
      .addMBB(TrueBB);

  NextMBBI = OrigBB.end();

  // Make sure live-ins are correctly attached to the new basic blocks.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *FalseBB);
  computeAndAddLiveIns(LiveRegs, *TrueBB);

  MI.eraseFromParent();
  return true;
}

// Look in the Target BB for a PHI node that references OrigBB. If found, we
// change it to a join from NewBB.
static void replacePhiBB(MachineFunction &MF, MachineBasicBlock *const TargetBB,
                         MachineBasicBlock *const OrigBB,
                         MachineBasicBlock *const NewBB) {
  assert(TargetBB != nullptr && OrigBB != nullptr && NewBB != nullptr);
  for (MachineInstr &Phi : *TargetBB) {
    if (!Phi.isPHI())
      return;

    // In a PHI node, operand 0 is the destination register. Remaining
    // operands are pairs of (register, predecessor block) as incoming edges.
    for (unsigned OpCtr = 1, NumOperands = Phi.getNumOperands();
         OpCtr < NumOperands; OpCtr += 2) {
      auto &BBOperand = Phi.getOperand(OpCtr + 1);
      if (BBOperand.getMBB() == OrigBB) {
        BBOperand.setMBB(NewBB);
        break;
      }
    }
  }
}

// Look in the Target BB for a PHI node that references OrigBB. If found, we add
// an additional join from NewBB.
static void addPhiBB(MachineFunction &MF, MachineBasicBlock *const TargetBB,
                     MachineBasicBlock *const OrigBB,
                     MachineBasicBlock *const NewBB) {
  assert(TargetBB != nullptr && OrigBB != nullptr && NewBB != nullptr);
  for (MachineInstr &Phi : *TargetBB) {
    if (!Phi.isPHI())
      return;

    for (unsigned OpCtr = 1, NumOperands = Phi.getNumOperands();
         OpCtr < NumOperands; OpCtr += 2) {
      auto &BBOperand = Phi.getOperand(OpCtr + 1);
      if (BBOperand.getMBB() == OrigBB) {
        // Create an operand which *also* refers to NewBB .
        Register Reg = Phi.getOperand(OpCtr).getReg();
        Phi.addOperand(MF, MachineOperand::CreateReg(Reg, /*isDef=*/false));
        Phi.addOperand(MF, MachineOperand::CreateMBB(NewBB));
        break;
      }
    }
  }
}

bool RISCVKeysomExpand::expandBEQ(MachineBasicBlock &OrigBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBeq()) {
    return false;
  }
  // Original:
  //
  //   beq rs1, rs2, offset
  //
  // The replacement code should look like:
  //
  //      [... previous instrs ...]
  //      blt rs1, rs2, Neq
  //      blt rs2, rs1, Neq
  //      jal X0, offset
  //  Neq:
  //      [... later instrs ...]

  MachineFunction *const MF = OrigBB.getParent();
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected BEQ to have 3 operands");

  MachineBasicBlock *TrueSucc = nullptr;
  MachineBasicBlock *FalseSucc = nullptr;
  SmallVector<MachineOperand, 3> Cond;
  if (!TII_->analyzeBranch(OrigBB, /*out*/ TrueSucc, /*out*/ FalseSucc,
                           /*out*/ Cond,
                           /*AllowModify=*/false)) {
    // If there's no false branch, use the layout successor instead.
    if (FalseSucc == nullptr) {
      if (auto Next = std::next(OrigBB.getIterator());
          Next != OrigBB.getParent()->end()) {
        FalseSucc = &*Next;
      }
    }
    assert(FalseSucc != nullptr);

    MachineBranchProbabilityInfo &MBPI =
        getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();
    auto TrueProb = MBPI.getEdgeProbability(&OrigBB, TrueSucc);
    auto FalseProb = MBPI.getEdgeProbability(&OrigBB, FalseSucc);

    assert(Cond.size() == 3 && "Invalid branch condition!");
    assert(Cond[0].getImm() == RISCVCC::CondCode::COND_EQ);
    const std::array IsKilled{Cond[1].isKill(), Cond[2].isKill()};
    Cond[1].setIsKill(false);
    Cond[2].setIsKill(false);

    MachineBasicBlock *const GtBB =
        MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
    MF->insert(std::next(OrigBB.getIterator()), GtBB);

    TII_->removeBranch(OrigBB);

    Cond[0].setImm(RISCVCC::CondCode::COND_LT);
    TII_->insertBranch(OrigBB, /*true bb=*/FalseSucc, /*false bb=*/GtBB, Cond,
                       MI.getDebugLoc());

    OrigBB.removeSuccessor(TrueSucc);
    OrigBB.addSuccessor(GtBB, TrueProb);

    Cond[0].setImm(RISCVCC::CondCode::COND_LT);
    Cond[1].setIsKill(IsKilled[0]);
    Cond[2].setIsKill(IsKilled[1]);
    std::swap(Cond[1], Cond[2]);
    TII_->insertBranch(*GtBB, /*true bb=*/FalseSucc, /*false bb=*/TrueSucc,
                       Cond, MI.getDebugLoc());

    GtBB->addSuccessor(FalseSucc, FalseProb);
    GtBB->addSuccessor(TrueSucc, TrueProb);

    replacePhiBB(*MF, TrueSucc, &OrigBB, GtBB);
    addPhiBB(*MF, FalseSucc, &OrigBB, GtBB);

    LivePhysRegs LiveRegs;
    computeAndAddLiveIns(LiveRegs, *GtBB);
  }

  NextMBBI = OrigBB.end();
  return true;
}

bool RISCVKeysomExpand::expandBNE(MachineBasicBlock &OrigBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBne()) {
    return false;
  }
  // Original:
  //
  //   bne rs1, rs2, offset
  //
  // The replacement code should look like:
  //
  //   [... previous instrs ...]
  //   blt rs1, rs2, offset
  //   blt rs2, rs1, offset
  //   [... later instrs ...]

  MachineFunction *const MF = OrigBB.getParent();
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected BNE to have 3 operands");

  MachineBasicBlock *TrueSucc = nullptr;
  MachineBasicBlock *FalseSucc = nullptr;
  SmallVector<MachineOperand, 3> Cond;
  if (!TII_->analyzeBranch(OrigBB, /*out*/ TrueSucc, /*out*/ FalseSucc,
                           /*out*/ Cond,
                           /*AllowModify=*/false)) {
    // If there's no false branch, use the layout successor instead.
    if (FalseSucc == nullptr) {
      if (auto Next = std::next(OrigBB.getIterator());
          Next != OrigBB.getParent()->end()) {
        FalseSucc = &*Next;
      }
    }
    assert(FalseSucc != nullptr);

    MachineBranchProbabilityInfo &MBPI =
        getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();
    auto TrueProb = MBPI.getEdgeProbability(&OrigBB, TrueSucc);
    auto FalseProb = FalseSucc != nullptr
                         ? MBPI.getEdgeProbability(&OrigBB, FalseSucc)
                         : BranchProbability::getUnknown();

    assert(Cond.size() == 3 && "Invalid branch condition!");
    assert(Cond[0].getImm() == RISCVCC::CondCode::COND_NE);
    const std::array IsKilled{Cond[1].isKill(), Cond[2].isKill()};
    Cond[1].setIsKill(false);
    Cond[2].setIsKill(false);

    MachineBasicBlock *const GtEqBB =
        MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
    MF->insert(std::next(OrigBB.getIterator()), GtEqBB);

    TII_->removeBranch(OrigBB);

    Cond[0].setImm(RISCVCC::CondCode::COND_LT);
    TII_->insertBranch(OrigBB, /*true bb=*/TrueSucc, /*false bb=*/GtEqBB, Cond,
                       MI.getDebugLoc());
    OrigBB.removeSuccessor(FalseSucc);
    OrigBB.addSuccessor(GtEqBB, TrueProb);

    Cond[0].setImm(RISCVCC::CondCode::COND_LT);
    Cond[1].setIsKill(IsKilled[0]);
    Cond[2].setIsKill(IsKilled[1]);
    std::swap(Cond[1], Cond[2]);
    TII_->insertBranch(*GtEqBB, /*true bb=*/TrueSucc, /*false bb=*/FalseSucc,
                       Cond, MI.getDebugLoc());
    GtEqBB->addSuccessor(FalseSucc, FalseProb);
    GtEqBB->addSuccessor(TrueSucc, TrueProb);

    replacePhiBB(*MF, FalseSucc, &OrigBB, GtEqBB);
    addPhiBB(*MF, TrueSucc, &OrigBB, GtEqBB);

    LivePhysRegs LiveRegs;
    computeAndAddLiveIns(LiveRegs, *GtEqBB);
  }

  NextMBBI = OrigBB.end();
  return true;
}

// Used to expand the BGE and BGEU instructions.
bool RISCVKeysomExpand::expandBranchGreaterEqual(
    RISCVCC::CondCode CC, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI, MachineBasicBlock::iterator &NextMBBI) {
  // Original:
  //
  //   bge[u] rs1, rs2, offset
  //
  // The replacement code should look like:
  //
  //   blt[u] rs2, rs1, offset

  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected BGE[U] to have 3 operands");

  MachineBasicBlock *TrueSucc = nullptr;
  MachineBasicBlock *FalseSucc = nullptr;
  SmallVector<MachineOperand, 3> Cond;
  if (!TII_->analyzeBranch(MBB, /*out*/ TrueSucc, /*out*/ FalseSucc,
                           /*out*/ Cond, /*AllowModify=*/false)) {
    MachineBranchProbabilityInfo &MBPI =
        getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();
    auto TrueProb = MBPI.getEdgeProbability(&MBB, TrueSucc);
    auto FalseProb = FalseSucc != nullptr
                         ? MBPI.getEdgeProbability(&MBB, FalseSucc)
                         : BranchProbability::getUnknown();

    assert(Cond.size() == 3 && "Invalid branch condition!");
    assert(Cond[0].getImm() == RISCVCC::CondCode::COND_GE ||
           Cond[0].getImm() == RISCVCC::CondCode::COND_GEU);
    assert(CC == RISCVCC::CondCode::COND_LT ||
           CC == RISCVCC::CondCode::COND_LTU);

    TII_->removeBranch(MBB);

    Cond[0].setImm(CC);
    std::swap(Cond[1], Cond[2]);
    TII_->insertBranch(MBB, /*true bb=*/TrueSucc, /*false bb=*/FalseSucc, Cond,
                       MI.getDebugLoc());

    if (MBB.hasSuccessorProbabilities()) {
      for (auto It = MBB.succ_begin(), End = MBB.succ_end(); It != End; ++It) {
        MachineBasicBlock *const Succ = *It;
        if (Succ == TrueSucc && FalseProb != BranchProbability::getUnknown()) {
          MBB.setSuccProbability(It, FalseProb);
        } else if (Succ == FalseSucc &&
                   TrueProb != BranchProbability::getUnknown()) {
          MBB.setSuccProbability(It, TrueProb);
        }
      }
    }
  }

  NextMBBI = MBB.end();
  return true;
}

bool RISCVKeysomExpand::expandBGE(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBge()) {
    return false;
  }
  return this->expandBranchGreaterEqual(RISCVCC::CondCode::COND_LT, MBB, MBBI,
                                        NextMBBI);
}

bool RISCVKeysomExpand::expandBGEU(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBgeu()) {
    return false;
  }
  return this->expandBranchGreaterEqual(RISCVCC::CondCode::COND_LTU, MBB, MBBI,
                                        NextMBBI);
}

bool RISCVKeysomExpand::expandSRAI(MachineBasicBlock &OrigBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoSrai()) {
    return false;
  }
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SRAI to have 3 operands");
  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  int64_t ShAmt = MI.getOperand(2).getImm();

  MachineRegisterInfo &MRI = OrigBB.getParent()->getRegInfo();
  InstructionHelper Helper{
      MRI, MRI.getRegClass(Rd), OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};
  Helper.rvSrai(Rd, Rs1, ShAmt);
  MI.eraseFromParent();
  return true;
}

} // end anonymous namespace

INITIALIZE_PASS(RISCVKeysomExpand, "riscv-keysom-expand", RISCV_KEYSOM_EXPAND,
                false, false)

namespace llvm {

FunctionPass *createRISCVKeysomExpandPass(bool IsPreRA) {
  return new RISCVKeysomExpand{IsPreRA};
}

} // end of namespace llvm
