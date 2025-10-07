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
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

#define DEBUG_TYPE "keysom-expand"
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
  [[nodiscard]] Register rvAdd(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::ADD, Rs1, Rs2);
  }
  [[nodiscard]] Register rvAdd(MachineOperand Op1, MachineOperand Op2) {
    Register Rd = MRI_.createVirtualRegister(RegClass_);
    auto Instruction = Op2.isReg() ? RISCV::ADD : RISCV::ADDI;
    BuildMI(OrigBB_, MBBI_, DL_, TII_->get(Instruction), Rd).add(Op1).add(Op2);
    return Rd;
  }
  [[nodiscard]] Register rvAddi(Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(true, RISCV::ADDI, RISCV::ADDI, Rs1, Immediate);
  }
  void rvAddi(Register Rd, Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    this->buildImmediate(true, RISCV::ADDI, RISCV::ADDI, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvSub(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SUB, Rs1, Rs2);
  }
  void rvSub(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::SUB, Rd, Rs1, Rs2);
  }

  [[nodiscard]] Register rvXori(Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(!STI_->hasVendorXKeysomNoXori(), RISCV::XORI,
                                RISCV::XOR, Rs1, Immediate);
  }
  [[nodiscard]] Register rvNot(Register Rs1) { return this->rvXori(Rs1, -1); }
  void rvXori(Register Rd, Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
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
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(!STI_->hasVendorXKeysomNoAndi(), RISCV::ANDI,
                                RISCV::AND, Rs1, Immediate);
  }
  void rvAndi(Register Rd, Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(!STI_->hasVendorXKeysomNoAndi(), RISCV::ANDI,
                                RISCV::AND, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvLui(int64_t Imm) {
    Register Rd = MRI_.createVirtualRegister(RegClass_);
    BuildMI(OrigBB_, MBBI_, DL_, TII_->get(RISCV::LUI), Rd).addImm(Imm);
    return Rd;
  }

  [[nodiscard]] Register rvLh(int64_t Offset, Register Rs1) {
    // TODO: check that lh is available!
    Register Rd = MRI_.createVirtualRegister(RegClass_);
    BuildMI(OrigBB_, MBBI_, DL_, TII_->get(RISCV::LH), Rd)
        .addReg(Rs1)
        .addImm(Offset);
    return Rd;
  }

  MachineInstrBuilder rvSh(Register Rs2, int64_t Offset, Register Rs1,
                           Align Alignment = Align(2)) {
    // TODO: check that sh is available!
    auto *const Function = OrigBB_.getParent();
    auto MMO = Function->getMachineMemOperand(
        MachinePointerInfo(), MachineMemOperand::MOStore, 2, Alignment);
    return BuildMI(OrigBB_, MBBI_, DL_, TII_->get(RISCV::SH))
        .addReg(Rs2)
        .addReg(Rs1)
        .addImm(Offset)
        .addMemOperand(MMO);
  }

  [[nodiscard]] Register rvOr(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::OR, Rs1, Rs2);
  }
  [[nodiscard]] Register rvOri(Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(!STI_->hasVendorXKeysomNoOri(), RISCV::ORI,
                                RISCV::OR, Rs1, Immediate);
  }
  void rvOri(Register Rd, Register Rs1, int64_t Immediate) {
    check12BitSextImmediate(Immediate);
    return this->buildImmediate(!STI_->hasVendorXKeysomNoOri(), RISCV::ORI,
                                RISCV::OR, Rd, Rs1, Immediate);
  }

  [[nodiscard]] Register rvSll(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SLL, Rs1, Rs2);
  }
  [[nodiscard]] Register rvSlli(Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
    if (ShAmt == 0) {
      return Rs1;
    }
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSlli(), RISCV::SLLI,
                                RISCV::SLL, Rs1, ShAmt);
  }
  void rvSlli(Register Rd, Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSlli(), RISCV::SLLI,
                                RISCV::SLL, Rd, Rs1, ShAmt);
  }

  [[nodiscard]] Register rvSra(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SRA, Rs1, Rs2);
  }
  void rvSra(Register Rd, Register Rs1, Register Rs2) {
    this->buildTwoReg(RISCV::SRA, Rd, Rs1, Rs2);
  }

  [[nodiscard]] Register rvSrai(Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSrai(), RISCV::SRAI,
                                RISCV::SRA, Rs1, ShAmt);
  }
  void rvSrai(Register Rd, Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
    this->buildImmediate(!STI_->hasVendorXKeysomNoSrai(), RISCV::SRAI,
                         RISCV::SRA, Rd, Rs1, ShAmt);
  }

  [[nodiscard]] Register rvSltu(Register Rs1, Register Rs2) {
    return this->buildTwoReg(RISCV::SLTU, Rs1, Rs2);
  }

  [[nodiscard]] Register rvSrli(Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
    return this->buildImmediate(!STI_->hasVendorXKeysomNoSrli(), RISCV::SRLI,
                                RISCV::SRL, Rs1, ShAmt);
  }
  void rvSrli(Register Rd, Register Rs1, int64_t ShAmt) {
    assert(ShAmt < (1 << 5) && "immediate value is too large!");
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
  constexpr void check12BitSextImmediate(int64_t Immediate) {
    (void)Immediate;
    assert(Immediate < (1U << 11) ||
           Immediate >= -(1 << 11) && "12-bit immediate value out of range!");
  }
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
  bool expandSB(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandSH(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandLB(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI, bool IsLBU);
  bool expandLH(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI, bool IsLHU);

  bool expandBranchGreaterEqual(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI,
                                MachineBasicBlock::iterator &NextMBBI);
  bool expandSetLessThan(unsigned CC, MachineBasicBlock &OrigBB,
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

#if 0 // Restore this check if there's doubt that legal code is produced.
  if (Modified) {
    verifyMachineFunction("keysom expand", MF);
  }
#endif

  return Modified;
}

bool RISCVKeysomExpand::expandMBB(MachineBasicBlock &MBB) {
  bool AnyModified = false;

  bool Modified = false;
  do {
    Modified = false;
    LLVM_DEBUG(dbgs() << "start basic block iteration (" << MBB.getFullName()
                      << ")\n");
    MachineBasicBlock::iterator MBBIt = MBB.begin(), E = MBB.end();
    while (MBBIt != E) {
      auto NextMBBIt = std::next(MBBIt);
      Modified |= expandMI(MBB, MBBIt, NextMBBIt);
      MBBIt = NextMBBIt;
    }
    AnyModified |= Modified;
  } while (Modified);
  LLVM_DEBUG(dbgs() << "completed basic block\n");
  return AnyModified;
}

bool RISCVKeysomExpand::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  LLVM_DEBUG(dbgs() << "expanding: " << TII_->getName(MBBI->getOpcode())
                    << '\n');
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
  case RISCV::SLTU:
  case RISCV::PseudoSLTU:
    return expandSLTU(MBB, MBBI, NextMBBI);
  case RISCV::SRLI:
    return expandSRLI(MBB, MBBI, NextMBBI);
  case RISCV::SRL:
    return expandSRL(MBB, MBBI, NextMBBI);
  case RISCV::SRAI:
    return expandSRAI(MBB, MBBI, NextMBBI);
  case RISCV::LB:
    return expandLB(MBB, MBBI, NextMBBI, /*IsLBU=*/false);
  case RISCV::LBU:
    return expandLB(MBB, MBBI, NextMBBI, /*IsLBU=*/true);
  case RISCV::LH:
    return expandLH(MBB, MBBI, NextMBBI, /*IsLHU=*/false);
  case RISCV::LHU:
    return expandLH(MBB, MBBI, NextMBBI, /*IsLHU=*/true);
  case RISCV::SB:
    return expandSB(MBB, MBBI, NextMBBI);
  case RISCV::SH:
    return expandSH(MBB, MBBI, NextMBBI);
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
  if (!IsPreRA_) {
    assert(false && "Can't expand SRLI after register allocation");
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
  // <2^11 is used here (the andi immediate field is 12 bits) so that the
  // immediate value remains positive.
  if (Mask < 1U << 11) {
    Helper.rvAndi(Rd, ShiftA, Mask);
  } else {
    Register UpperImm = MRI.createVirtualRegister(MRI.getRegClass(Rd));
    BuildMI(OrigBB, MBBI, DL, TII_->get(RISCV::LUI), UpperImm)
        .addImm((Mask >> 12) + 1U);
    auto FullMask = Helper.rvAddi(UpperImm, -1);
    Helper.rvAnd(Rd, ShiftA, FullMask);
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
  return this->expandSetLessThan(RISCV::BLT, OrigBB, MBBI, NextMBBI);
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
  return this->expandSetLessThan(RISCV::BLTU, OrigBB, MBBI, NextMBBI);
}

bool RISCVKeysomExpand::expandSetLessThan(
    unsigned CC, MachineBasicBlock &OrigBB,
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

  assert(CC == RISCV::BLT || CC == RISCV::BLTU);
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

static bool setKill(MachineOperand &Op, bool const K) {
  bool WasKill = false;
  if (Op.isReg()) {
    WasKill = Op.isKill();
    Op.setIsKill(K);
  }
  return WasKill;
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
    assert(Cond[0].getImm() == RISCV::BEQ);
    const std::array IsKilled{setKill(Cond[1], false), setKill(Cond[2], false)};

    MachineBasicBlock *const GtBB =
        MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
    MF->insert(std::next(OrigBB.getIterator()), GtBB);

    TII_->removeBranch(OrigBB);

    Cond[0].setImm(RISCV::BLT);
    TII_->insertBranch(OrigBB, /*true bb=*/FalseSucc, /*false bb=*/GtBB, Cond,
                       MI.getDebugLoc());

    OrigBB.removeSuccessor(TrueSucc);
    OrigBB.addSuccessor(GtBB, TrueProb);

    Cond[0].setImm(RISCV::BLT);
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
    assert(Cond[0].getImm() == RISCV::BNE);
    const std::array IsKilled{Cond[1].isKill(), Cond[2].isKill()};
    Cond[1].setIsKill(false);
    Cond[2].setIsKill(false);

    MachineBasicBlock *const GtEqBB =
        MF->CreateMachineBasicBlock(OrigBB.getBasicBlock());
    MF->insert(std::next(OrigBB.getIterator()), GtEqBB);

    TII_->removeBranch(OrigBB);

    Cond[0].setImm(RISCV::BLT);
    TII_->insertBranch(OrigBB, /*true bb=*/TrueSucc, /*false bb=*/GtEqBB, Cond,
                       MI.getDebugLoc());
    OrigBB.removeSuccessor(FalseSucc);
    OrigBB.addSuccessor(GtEqBB, TrueProb);

    Cond[0].setImm(RISCV::BLT);
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
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
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
  if (TII_->analyzeBranch(MBB, /*out*/ TrueSucc, /*out*/ FalseSucc,
                          /*out*/ Cond, /*AllowModify=*/false)) {
    assert(false);
    return false;
  }
  if (FalseSucc == nullptr && &MBB != &MBB.getParent()->back()) {
    FalseSucc = &*std::next(MBB.getIterator()); // Use the layout successor
  }
  assert(TrueSucc != nullptr);

  MachineBranchProbabilityInfo &MBPI =
      getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();

  auto TrueProb = MBPI.getEdgeProbability(&MBB, TrueSucc);
  auto FalseProb = FalseSucc != nullptr
                       ? MBPI.getEdgeProbability(&MBB, FalseSucc)
                       : BranchProbability::getUnknown();

  assert(Cond.size() == 3 && "Invalid branch condition!");
  assert(Cond[0].getImm() == RISCV::BGE || Cond[0].getImm() == RISCV::BGEU);

  bool const Replaced = !TII_->reverseBranchCondition(Cond);
  (void)Replaced;
  assert(Replaced);

  TII_->removeBranch(MBB);
  std::swap(TrueSucc, FalseSucc);
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
      } else {
        assert(0);
      }
    }
  }
  return true;
}

bool RISCVKeysomExpand::expandBGE(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBge()) {
    return false;
  }
  return this->expandBranchGreaterEqual(MBB, MBBI, NextMBBI);
}

bool RISCVKeysomExpand::expandBGEU(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoBgeu()) {
    return false;
  }
  return this->expandBranchGreaterEqual(MBB, MBBI, NextMBBI);
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

void updateMemOperands(MachineInstr *const Instruction,
                       MachineMemOperand const &PrevMemOperand,
                       uint64_t NewOffset, LocationSize NewSize,
                       MachineMemOperand::Flags NewFlags) {
  assert(Instruction != nullptr);
  assert(Instruction->memoperands_empty());

  MachineFunction &Function = *Instruction->getParent()->getParent();
  MachinePointerInfo PtrInfo = PrevMemOperand.getPointerInfo();
  PtrInfo.Offset = NewOffset;
  std::array<MachineMemOperand *, 1> NewMemRefs{{Function.getMachineMemOperand(
      PtrInfo, NewFlags,
      NewSize, // Updated size
      PrevMemOperand.getBaseAlign(), PrevMemOperand.getAAInfo(),
      PrevMemOperand.getRanges(), PrevMemOperand.getSyncScopeID(),
      PrevMemOperand.getSuccessOrdering(),
      PrevMemOperand.getFailureOrdering())}};
  Instruction->setMemRefs(Function, NewMemRefs);
}

MachineMemOperand const &getMachineMemOperand(MachineInstr const &MI) {
  assert(MI.hasOneMemOperand());
  MachineMemOperand const *const MMO = *MI.memoperands_begin();
  return *MMO;
}

std::tuple<uint64_t, LocationSize, MachineMemOperand::Flags>
getStoreMemOperands(MachineInstr const &MI) {
  assert(MI.hasOneMemOperand());
  MachineMemOperand const &MMO = getMachineMemOperand(MI);
  return {MMO.getPointerInfo().Offset, MMO.getSize(), MMO.getFlags()};
}

constexpr MachineMemOperand::Flags
memFlagsForStore(MachineMemOperand::Flags Flags) {
  return (Flags & ~MachineMemOperand::Flags::MOLoad) |
         MachineMemOperand::Flags::MOStore;
}
constexpr MachineMemOperand::Flags
memFlagsForLoad(MachineMemOperand::Flags Flags) {
  return (Flags & ~MachineMemOperand::Flags::MOStore) |
         MachineMemOperand::Flags::MOLoad;
}

enum class AlignType { unknown, odd, even };

static AlignType getAlignmentKind(MachineInstr const &MI,
                                  MachineFunction const &Function) {
  auto OpAligned = AlignType::unknown;

  // Code here attempts to determine whether the store is to an address that is
  // known to be even or odd aligned.
  for (MachineMemOperand const *const MemOperand : MI.memoperands()) {
    if (Align Alignment = MemOperand->getAlign(); Alignment > Align(1)) {
      const int64_t Offset = MemOperand->getOffset();
      OpAligned = (Offset > 0 && (static_cast<uint64_t>(Offset) & 1) != 0)
                      ? AlignType::odd
                      : AlignType::even;
    }
  }

  DataLayout const &Layout = Function.getDataLayout();
  auto &Op1 = MI.getOperand(1);
  auto &Op2 = MI.getOperand(2);

  // In the case of a store to a global structure, we can do better by
  // examining the alignment of the struct itself and looking at the
  // offset of the store within it.
  if (Op1.isReg() && Op2.isGlobal()) {
    if (const GlobalVariable *const GVar =
            dyn_cast<GlobalVariable>(Op2.getGlobal())) {
      if (StructType *const ST = dyn_cast<StructType>(GVar->getValueType())) {
        const StructLayout *const SL = Layout.getStructLayout(ST);
        if (SL->getAlignment() > Align(1)) {
          OpAligned =
              (Op2.getOffset() & 1) == 0 ? AlignType::even : AlignType::odd;
        }
      }
    }
  }
  return OpAligned;
}

bool RISCVKeysomExpand::expandSB(MachineBasicBlock &OrigBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoSb()) {
    return false;
  }
  if (!IsPreRA_) {
    assert(false && "Can't expand SB after register allocation");
    return false;
  }
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SB to have 3 operands");
  Register Rs2 = MI.getOperand(0).getReg(); // destination is always a register.

  MachineFunction *const Function = OrigBB.getParent();
  MachineRegisterInfo &MRI = Function->getRegInfo();
  const TargetRegisterClass *const DestRegisterClass = MRI.getRegClass(Rs2);
  InstructionHelper Helper{
      MRI, DestRegisterClass, OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};

  // (TODO: the andi rs2b,rs2,255 instruction is not necessary here since 'sh'
  // will ignore bits in the upper half of the word.)
  Register Rs2b = Helper.rvAndi(Rs2, 0xFF);

  AlignType StoreAligned = getAlignmentKind(MI, *Function);
  DataLayout const &Layout = Function->getDataLayout();
  auto &Op1 = MI.getOperand(1);
  auto &Op2 = MI.getOperand(2);

  bool const WasKill = setKill(Op1, false);

  auto [MOOffset, MOSize, MOFlags] = getStoreMemOperands(MI);
  MOOffset &= ~1;
  MOSize = MOSize.unionWith(LocationSize::precise(2));
  MachineInstr *SHInstr = nullptr;

  if (StoreAligned != AlignType::unknown) {
    if (StoreAligned == AlignType::odd && Op2.isGlobal()) {
      Op2.setOffset(Op2.getOffset() & ~1U);
    }
    Register Value = MRI.createVirtualRegister(DestRegisterClass);
    MachineInstr *const LHInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::LH), Value)
            .add(Op1)
            .add(Op2)
            .getInstr();
    updateMemOperands(LHInstr, getMachineMemOperand(MI), MOOffset, MOSize,
                      memFlagsForLoad(MOFlags));

    Register NewValue;
    // TODO: invert for big-endian?
    if (StoreAligned == AlignType::even) {
      NewValue = Helper.rvOr(Helper.rvAndi(Value, -256), Rs2b);
    } else {
      NewValue =
          Helper.rvOr(Helper.rvAndi(Value, 0xFFU), Helper.rvSlli(Rs2b, 8));
    }

    SHInstr = BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::SH))
                  .addReg(NewValue, RegState::Kill)
                  .add(Op1)
                  .add(Op2)
                  .getInstr();
  } else {
    Register Addr = Helper.rvAdd(Op1, Op2);
    Register AlignedAddr = Helper.rvAndi(Addr, ~1);
    Register LHValue = MRI.createVirtualRegister(DestRegisterClass);
    MachineInstr *const LHInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::LH), LHValue)
            .addReg(AlignedAddr)
            .addImm(0)
            .getInstr();
    updateMemOperands(LHInstr, getMachineMemOperand(MI), MOOffset, MOSize,
                      memFlagsForLoad(MOFlags));

    Register Shift =
        Helper.rvSltu(AlignedAddr, Addr); // Shift = AlignedAddr < Addr
    if (Layout.isBigEndian()) {
      Shift = Helper.rvXori(Shift, 1); // Shift = !Shift
    }
    Shift = Helper.rvSlli(Shift, 3); // Shift *= 8

    // mask is 0x00FF (even) or 0xFF00 (odd)
    Register Mask =
        Helper.rvXori(Helper.rvSll(Helper.rvAddi(RISCV::X0, 0xFF), Shift), -1);
    Register Rs2Shifted = Helper.rvSll(Rs2b, Shift);
    Register SHValue = Helper.rvOr(Helper.rvAnd(LHValue, Mask), Rs2Shifted);

    SHInstr = BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::SH))
                  .addReg(SHValue, RegState::Kill)
                  .addReg(AlignedAddr, RegState::Kill)
                  .addImm(0)
                  .getInstr();
  }
  updateMemOperands(SHInstr, getMachineMemOperand(MI), MOOffset, MOSize,
                    memFlagsForStore(MOFlags));
  setKill(Op1, WasKill); // Restore the original kill state.

  MI.eraseFromParent();
  return true;
}

bool RISCVKeysomExpand::expandLB(MachineBasicBlock &OrigBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI,
                                 bool IsLBU) {
  if (IsLBU && !STI_->hasVendorXKeysomNoLbu()) {
    return false;
  }
  if (!IsLBU && !STI_->hasVendorXKeysomNoLb()) {
    return false;
  }
  if (!IsPreRA_) {
    assert(false && "Can't expand LB/LBU after register allocation");
    return false;
  }

  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected LB/LBU to have 3 operands");
  Register Rs2 = MI.getOperand(0).getReg(); // destination is always a register.

  MachineFunction *const Function = OrigBB.getParent();
  MachineRegisterInfo &MRI = Function->getRegInfo();
  const TargetRegisterClass *const DestRegisterClass = MRI.getRegClass(Rs2);
  InstructionHelper Helper{
      MRI, DestRegisterClass, OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};

  const AlignType LoadAligned = getAlignmentKind(MI, *Function);
  auto &Op1 = MI.getOperand(1);
  auto &Op2 = MI.getOperand(2);

  auto [MOOffset, MOSize, MOFlags] = getStoreMemOperands(MI);
  MOSize = MOSize.unionWith(LocationSize::precise(2));

  const MCInstrDesc &LoadInstruction =
      TII_->get(IsLBU ? RISCV::LHU : RISCV::LH);

  Register LoadResult{};
  int64_t ShAmt = 24;
  MachineInstr *LHInstr = nullptr;
  if (LoadAligned == AlignType::odd) {
    if (Op2.isGlobal()) {
      Op2.setOffset(Op2.getOffset() & ~1U);
    }
    LoadResult = MRI.createVirtualRegister(DestRegisterClass);
    LHInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), LoadInstruction, LoadResult)
            .add(Op1)
            .add(Op2)
            .getInstr();
    // The value is in bits 8-15.
    ShAmt = 16;
  } else if (LoadAligned == AlignType::even) {
    LoadResult = MRI.createVirtualRegister(DestRegisterClass);
    LHInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), LoadInstruction, LoadResult)
            .add(Op1)
            .add(Op2)
            .getInstr();
  } else {
    Register Addr = Helper.rvAdd(Op1, Op2);
    Register AlignedAddr = Helper.rvAndi(Addr, ~1);
    Register LHValue = MRI.createVirtualRegister(DestRegisterClass);
    LHInstr = BuildMI(OrigBB, MBBI, MI.getDebugLoc(), LoadInstruction, LHValue)
                  .addReg(AlignedAddr)
                  .addImm(0)
                  .getInstr();
    // Shift = AlignedAddr < Addr = 1(odd)/0(even)
    Register Shift = Helper.rvSltu(AlignedAddr, Addr);
    Register Shift8 = Helper.rvSlli(Shift, 3);  // Shift *= 8
    LoadResult = Helper.rvSrl(LHValue, Shift8); // Rs2 = LhValue << Shift8
  }
  updateMemOperands(LHInstr, getMachineMemOperand(MI), MOOffset & ~1, MOSize,
                    memFlagsForLoad(MOFlags));

  if (IsLBU) {
    // Zero extend
    if (LoadAligned == AlignType::odd) {
      Helper.rvAndi(Rs2, Helper.rvSrli(LoadResult, 8), 0xFF);
    } else {
      Helper.rvAndi(Rs2, LoadResult, 0xFF);
    }
  } else {
    // Now do the sign extension.
    Register ShiftToTop =
        Helper.rvSlli(LoadResult, ShAmt); // move bit 7 to bit 31
    // Arithmetic shift right fills with sign
    Helper.rvSrai(Rs2, ShiftToTop, 24);
  }

  MI.eraseFromParent();
  return true;
}

bool RISCVKeysomExpand::expandSH(MachineBasicBlock &OrigBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  if (!STI_->hasVendorXKeysomNoSh()) {
    return false;
  }
  if (!IsPreRA_) {
    assert(false && "Can't expand SH after register allocation");
    return false;
  }
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SH to have 3 operands");
  Register Rs2 = MI.getOperand(0).getReg();

  MachineFunction *const Function = OrigBB.getParent();
  MachineRegisterInfo &MRI = Function->getRegInfo();
  InstructionHelper Helper{
      MRI, MRI.getRegClass(Rs2), OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};

  Register MaskUpper = Helper.rvLui(16);
  Register MaskFFFF = Helper.rvAddi(MaskUpper, -1);
  Register Rs2b = Helper.rvAnd(MaskFFFF, Rs2);

  AlignType StoreAligned = AlignType::unknown;
  DataLayout const &Layout = Function->getDataLayout();
  auto &Op1 = MI.getOperand(1);
  auto &Op2 = MI.getOperand(2);

  bool const WasKill = setKill(Op1, false);

  auto [MOOffset, MOSize, MOFlags] = getStoreMemOperands(MI);
  MOOffset &= ~0b11;
  MOSize = MOSize.unionWith(LocationSize::precise(4));
  MachineInstr *SWInstr = nullptr;

  if (StoreAligned != AlignType::unknown) {
    assert(false); // TODO
  } else {
    Register Addr = Helper.rvAdd(Op1, Op2);
    Register AlignedAddr = Helper.rvAndi(Addr, ~0b11);
    Register LWValue = MRI.createVirtualRegister(MRI.getRegClass(Rs2));
    MachineInstr *const LWInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::LW), LWValue)
            .addReg(AlignedAddr)
            .addImm(0)
            .getInstr();
    updateMemOperands(LWInstr, getMachineMemOperand(MI), MOOffset, MOSize,
                      memFlagsForLoad(MOFlags));

    Register Shift =
        Helper.rvSltu(AlignedAddr, Addr); // Shift = AlignedAddr < Addr
    if (Layout.isBigEndian()) {
      Shift = Helper.rvXori(Shift, 1); // Shift = !Shift
    }
    Shift = Helper.rvSlli(Shift, 4); // Shift *= 16

    Register Mask = Helper.rvXori(Helper.rvSll(MaskFFFF, Shift), -1);
    Register Rs2Shifted = Helper.rvSll(Rs2b, Shift);
    Register SWValue = Helper.rvOr(Helper.rvAnd(LWValue, Mask), Rs2Shifted);

    SWInstr = BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::SW))
                  .addReg(SWValue, RegState::Kill)
                  .addReg(AlignedAddr, RegState::Kill)
                  .addImm(0)
                  .getInstr();
  }
  updateMemOperands(SWInstr, getMachineMemOperand(MI), MOOffset, MOSize,
                    memFlagsForStore(MOFlags));
  setKill(Op1, WasKill); // Restore the original kill state.

  MI.eraseFromParent();
  return true;
}

bool RISCVKeysomExpand::expandLH(MachineBasicBlock &OrigBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI,
                                 bool IsLHU) {
  if (IsLHU && !STI_->hasVendorXKeysomNoLhu()) {
    return false;
  }
  if (!IsLHU && !STI_->hasVendorXKeysomNoLh()) {
    return false;
  }
  if (!IsPreRA_) {
    assert(false && "Can't expand LH/LHU after register allocation");
    return false;
  }

  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected LH/LHU to have 3 operands");
  Register Rs2 = MI.getOperand(0).getReg(); // destination is always a register.

  MachineFunction *const Function = OrigBB.getParent();
  MachineRegisterInfo &MRI = Function->getRegInfo();
  InstructionHelper Helper{
      MRI, MRI.getRegClass(Rs2), OrigBB, MBBI, MI.getDebugLoc(), STI_, TII_};

  AlignType LoadAligned = AlignType::unknown; // getAlignmentKind(MI,
                                              // *Function);

  auto &Op1 = MI.getOperand(1);
  auto &Op2 = MI.getOperand(2);

  auto [MOOffset, MOSize, MOFlags] = getStoreMemOperands(MI);
  MOOffset &= ~0b11, MOSize = MOSize.unionWith(LocationSize::precise(4));

  Register Result{};
  if (LoadAligned == AlignType::odd) {
    assert(false); // TODO
  } else if (LoadAligned == AlignType::even) {
    assert(false); // TODO
  } else {
    Register Addr = Helper.rvAdd(Op1, Op2);
    Register AlignedAddr = Helper.rvAndi(Addr, ~0b11);
    Register LWValue = MRI.createVirtualRegister(MRI.getRegClass(Rs2));
    MachineInstr *const LWInstr =
        BuildMI(OrigBB, MBBI, MI.getDebugLoc(), TII_->get(RISCV::LW), LWValue)
            .addReg(AlignedAddr)
            .addImm(0)
            .getInstr();
    updateMemOperands(LWInstr, getMachineMemOperand(MI), MOOffset & ~0b11,
                      MOSize, memFlagsForLoad(MOFlags));

    Register Shift0 = Helper.rvSub(Addr, AlignedAddr); // shift is 0 or 2
    Register Shift = Helper.rvSlli(Shift0, 3);         // Shift *= 8

    Register LWValue2 =
        Helper.rvSrl(LWValue, Shift); // LWValue2 = LWValue >> Shift

    Result = LWValue2;
  }

  if (IsLHU) {
    // Zero extend
    if (LoadAligned == AlignType::odd) {
      assert(false); // TODO
    } else {
      Register MaxShortUpper = Helper.rvLui(16);
      Register MaxShort = Helper.rvAddi(MaxShortUpper, -1); // MaxShort = 0xFFFF

      Helper.rvAnd(Rs2, Result, MaxShort);
    }
  } else {
    // Now do the sign extension.
    Register ShiftToTop = Helper.rvSlli(Result, 16); // move bit 15 to bit 31
    Helper.rvSrai(Rs2, ShiftToTop,
                  16); // arithmetic shift right fills with sign
  }

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
