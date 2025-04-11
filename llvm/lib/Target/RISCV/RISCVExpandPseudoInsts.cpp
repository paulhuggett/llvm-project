//===-- RISCVExpandPseudoInsts.cpp - Expand pseudo instructions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/MCContext.h"

using namespace llvm;

#define RISCV_EXPAND_PSEUDO_NAME "RISC-V pseudo instruction expansion pass"
#define RISCV_PRERA_EXPAND_PSEUDO_NAME "RISC-V Pre-RA pseudo instruction expansion pass"

namespace {

class RISCVExpandPseudo : public MachineFunctionPass {
public:
  const RISCVSubtarget *STI;
  const RISCVInstrInfo *TII;
  static char ID;

  RISCVExpandPseudo() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return RISCV_EXPAND_PSEUDO_NAME; }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandCCOp(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandVMSET_VMCLR(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI, unsigned Opcode);
  bool expandMV_FPR16INX(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI);
  bool expandMV_FPR32INX(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI);
  bool expandRV32ZdinxStore(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI);
  bool expandRV32ZdinxLoad(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI);
  bool expandPseudoReadVLENBViaVSETVLIX0(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MBBI);
#ifndef NDEBUG
  unsigned getInstSizeInBytes(const MachineFunction &MF) const {
    unsigned Size = 0;
    for (auto &MBB : MF)
      for (auto &MI : MBB)
        Size += TII->getInstSizeInBytes(MI);
    return Size;
  }
#endif
};

char RISCVExpandPseudo::ID = 0;

bool RISCVExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  STI = &MF.getSubtarget<RISCVSubtarget>();
  TII = STI->getInstrInfo();

#ifndef NDEBUG
  const unsigned OldSize = getInstSizeInBytes(MF);
#endif

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);

#ifndef NDEBUG
  const unsigned NewSize = getInstSizeInBytes(MF);
  assert(OldSize >= NewSize);
#endif
  return Modified;
}

bool RISCVExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RISCVExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  // RISCVInstrInfo::getInstSizeInBytes expects that the total size of the
  // expanded instructions for each pseudo is correct in the Size field of the
  // tablegen definition for the pseudo.
  switch (MBBI->getOpcode()) {
  case RISCV::PseudoMV_FPR16INX:
    return expandMV_FPR16INX(MBB, MBBI);
  case RISCV::PseudoMV_FPR32INX:
    return expandMV_FPR32INX(MBB, MBBI);
  case RISCV::PseudoRV32ZdinxSD:
    return expandRV32ZdinxStore(MBB, MBBI);
  case RISCV::PseudoRV32ZdinxLD:
    return expandRV32ZdinxLoad(MBB, MBBI);
  case RISCV::PseudoCCMOVGPRNoX0:
  case RISCV::PseudoCCMOVGPR:
  case RISCV::PseudoCCADD:
  case RISCV::PseudoCCSUB:
  case RISCV::PseudoCCAND:
  case RISCV::PseudoCCOR:
  case RISCV::PseudoCCXOR:
  case RISCV::PseudoCCADDW:
  case RISCV::PseudoCCSUBW:
  case RISCV::PseudoCCSLL:
  case RISCV::PseudoCCSRL:
  case RISCV::PseudoCCSRA:
  case RISCV::PseudoCCADDI:
  case RISCV::PseudoCCSLLI:
  case RISCV::PseudoCCSRLI:
  case RISCV::PseudoCCSRAI:
  case RISCV::PseudoCCANDI:
  case RISCV::PseudoCCORI:
  case RISCV::PseudoCCXORI:
  case RISCV::PseudoCCSLLW:
  case RISCV::PseudoCCSRLW:
  case RISCV::PseudoCCSRAW:
  case RISCV::PseudoCCADDIW:
  case RISCV::PseudoCCSLLIW:
  case RISCV::PseudoCCSRLIW:
  case RISCV::PseudoCCSRAIW:
  case RISCV::PseudoCCANDN:
  case RISCV::PseudoCCORN:
  case RISCV::PseudoCCXNOR:
    return expandCCOp(MBB, MBBI, NextMBBI);
  case RISCV::PseudoVMCLR_M_B1:
  case RISCV::PseudoVMCLR_M_B2:
  case RISCV::PseudoVMCLR_M_B4:
  case RISCV::PseudoVMCLR_M_B8:
  case RISCV::PseudoVMCLR_M_B16:
  case RISCV::PseudoVMCLR_M_B32:
  case RISCV::PseudoVMCLR_M_B64:
    // vmclr.m vd => vmxor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXOR_MM);
  case RISCV::PseudoVMSET_M_B1:
  case RISCV::PseudoVMSET_M_B2:
  case RISCV::PseudoVMSET_M_B4:
  case RISCV::PseudoVMSET_M_B8:
  case RISCV::PseudoVMSET_M_B16:
  case RISCV::PseudoVMSET_M_B32:
  case RISCV::PseudoVMSET_M_B64:
    // vmset.m vd => vmxnor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXNOR_MM);
  case RISCV::PseudoReadVLENBViaVSETVLIX0:
    return expandPseudoReadVLENBViaVSETVLIX0(MBB, MBBI);
  }

  return false;
}

bool RISCVExpandPseudo::expandCCOp(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {

  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  MachineBasicBlock *TrueBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  MachineBasicBlock *MergeBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  MF->insert(++MBB.getIterator(), TrueBB);
  MF->insert(++TrueBB->getIterator(), MergeBB);

  // We want to copy the "true" value when the condition is true which means
  // we need to invert the branch condition to jump over TrueBB when the
  // condition is false.
  auto CC = static_cast<RISCVCC::CondCode>(MI.getOperand(3).getImm());
  CC = RISCVCC::getOppositeBranchCondition(CC);

  // Insert branch instruction.
  BuildMI(MBB, MBBI, DL, TII->getBrCond(CC))
      .addReg(MI.getOperand(1).getReg())
      .addReg(MI.getOperand(2).getReg())
      .addMBB(MergeBB);

  Register DestReg = MI.getOperand(0).getReg();
  assert(MI.getOperand(4).getReg() == DestReg);

  if (MI.getOpcode() == RISCV::PseudoCCMOVGPR ||
      MI.getOpcode() == RISCV::PseudoCCMOVGPRNoX0) {
    // Add MV.
    BuildMI(TrueBB, DL, TII->get(RISCV::ADDI), DestReg)
        .add(MI.getOperand(5))
        .addImm(0);
  } else {
    unsigned NewOpc;
    switch (MI.getOpcode()) {
    default:
      llvm_unreachable("Unexpected opcode!");
    case RISCV::PseudoCCADD:   NewOpc = RISCV::ADD;   break;
    case RISCV::PseudoCCSUB:   NewOpc = RISCV::SUB;   break;
    case RISCV::PseudoCCSLL:   NewOpc = RISCV::SLL;   break;
    case RISCV::PseudoCCSRL:   NewOpc = RISCV::SRL;   break;
    case RISCV::PseudoCCSRA:   NewOpc = RISCV::SRA;   break;
    case RISCV::PseudoCCAND:   NewOpc = RISCV::AND;   break;
    case RISCV::PseudoCCOR:    NewOpc = RISCV::OR;    break;
    case RISCV::PseudoCCXOR:   NewOpc = RISCV::XOR;   break;
    case RISCV::PseudoCCADDI:  NewOpc = RISCV::ADDI;  break;
    case RISCV::PseudoCCSLLI:  NewOpc = RISCV::SLLI;  break;
    case RISCV::PseudoCCSRLI:  NewOpc = RISCV::SRLI;  break;
    case RISCV::PseudoCCSRAI:  NewOpc = RISCV::SRAI;  break;
    case RISCV::PseudoCCANDI:  NewOpc = RISCV::ANDI;  break;
    case RISCV::PseudoCCORI:   NewOpc = RISCV::ORI;   break;
    case RISCV::PseudoCCXORI:  NewOpc = RISCV::XORI;  break;
    case RISCV::PseudoCCADDW:  NewOpc = RISCV::ADDW;  break;
    case RISCV::PseudoCCSUBW:  NewOpc = RISCV::SUBW;  break;
    case RISCV::PseudoCCSLLW:  NewOpc = RISCV::SLLW;  break;
    case RISCV::PseudoCCSRLW:  NewOpc = RISCV::SRLW;  break;
    case RISCV::PseudoCCSRAW:  NewOpc = RISCV::SRAW;  break;
    case RISCV::PseudoCCADDIW: NewOpc = RISCV::ADDIW; break;
    case RISCV::PseudoCCSLLIW: NewOpc = RISCV::SLLIW; break;
    case RISCV::PseudoCCSRLIW: NewOpc = RISCV::SRLIW; break;
    case RISCV::PseudoCCSRAIW: NewOpc = RISCV::SRAIW; break;
    case RISCV::PseudoCCANDN:  NewOpc = RISCV::ANDN;  break;
    case RISCV::PseudoCCORN:   NewOpc = RISCV::ORN;   break;
    case RISCV::PseudoCCXNOR:  NewOpc = RISCV::XNOR;  break;
    }
    BuildMI(TrueBB, DL, TII->get(NewOpc), DestReg)
        .add(MI.getOperand(5))
        .add(MI.getOperand(6));
  }

  TrueBB->addSuccessor(MergeBB);

  MergeBB->splice(MergeBB->end(), &MBB, MI, MBB.end());
  MergeBB->transferSuccessors(&MBB);

  MBB.addSuccessor(TrueBB);
  MBB.addSuccessor(MergeBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *TrueBB);
  computeAndAddLiveIns(LiveRegs, *MergeBB);

  return true;
}

bool RISCVExpandPseudo::expandVMSET_VMCLR(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          unsigned Opcode) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register DstReg = MBBI->getOperand(0).getReg();
  const MCInstrDesc &Desc = TII->get(Opcode);
  BuildMI(MBB, MBBI, DL, Desc, DstReg)
      .addReg(DstReg, RegState::Undef)
      .addReg(DstReg, RegState::Undef);
  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandMV_FPR16INX(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register DstReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(0).getReg(), RISCV::sub_16, &RISCV::GPRRegClass);
  Register SrcReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(1).getReg(), RISCV::sub_16, &RISCV::GPRRegClass);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), DstReg)
      .addReg(SrcReg, getKillRegState(MBBI->getOperand(1).isKill()))
      .addImm(0);

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandMV_FPR32INX(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register DstReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(0).getReg(), RISCV::sub_32, &RISCV::GPRRegClass);
  Register SrcReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(1).getReg(), RISCV::sub_32, &RISCV::GPRRegClass);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), DstReg)
      .addReg(SrcReg, getKillRegState(MBBI->getOperand(1).isKill()))
      .addImm(0);

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

// This function expands the PseudoRV32ZdinxSD for storing a double-precision
// floating-point value into memory by generating an equivalent instruction
// sequence for RV32.
bool RISCVExpandPseudo::expandRV32ZdinxStore(MachineBasicBlock &MBB,
                                             MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register Lo =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_even);
  Register Hi =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_odd);
  if (Hi == RISCV::DUMMY_REG_PAIR_WITH_X0)
    Hi = RISCV::X0;

  auto MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                   .addReg(Lo, getKillRegState(MBBI->getOperand(0).isKill()))
                   .addReg(MBBI->getOperand(1).getReg())
                   .add(MBBI->getOperand(2));

  MachineInstrBuilder MIBHi;
  if (MBBI->getOperand(2).isGlobal() || MBBI->getOperand(2).isCPI()) {
    assert(MBBI->getOperand(2).getOffset() % 8 == 0);
    MBBI->getOperand(2).setOffset(MBBI->getOperand(2).getOffset() + 4);
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                .addReg(Hi, getKillRegState(MBBI->getOperand(0).isKill()))
                .add(MBBI->getOperand(1))
                .add(MBBI->getOperand(2));
  } else {
    assert(isInt<12>(MBBI->getOperand(2).getImm() + 4));
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                .addReg(Hi, getKillRegState(MBBI->getOperand(0).isKill()))
                .add(MBBI->getOperand(1))
                .addImm(MBBI->getOperand(2).getImm() + 4);
  }

  MachineFunction *MF = MBB.getParent();
  SmallVector<MachineMemOperand *> NewLoMMOs;
  SmallVector<MachineMemOperand *> NewHiMMOs;
  for (const MachineMemOperand *MMO : MBBI->memoperands()) {
    NewLoMMOs.push_back(MF->getMachineMemOperand(MMO, 0, 4));
    NewHiMMOs.push_back(MF->getMachineMemOperand(MMO, 4, 4));
  }
  MIBLo.setMemRefs(NewLoMMOs);
  MIBHi.setMemRefs(NewHiMMOs);

  MBBI->eraseFromParent();
  return true;
}

// This function expands PseudoRV32ZdinxLoad for loading a double-precision
// floating-point value from memory into an equivalent instruction sequence for
// RV32.
bool RISCVExpandPseudo::expandRV32ZdinxLoad(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register Lo =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_even);
  Register Hi =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_odd);
  assert(Hi != RISCV::DUMMY_REG_PAIR_WITH_X0 && "Cannot write to X0_Pair");

  MachineInstrBuilder MIBLo, MIBHi;

  // If the register of operand 1 is equal to the Lo register, then swap the
  // order of loading the Lo and Hi statements.
  bool IsOp1EqualToLo = Lo == MBBI->getOperand(1).getReg();
  // Order: Lo, Hi
  if (!IsOp1EqualToLo) {
    MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Lo)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
  }

  if (MBBI->getOperand(2).isGlobal() || MBBI->getOperand(2).isCPI()) {
    auto Offset = MBBI->getOperand(2).getOffset();
    assert(Offset % 8 == 0);
    MBBI->getOperand(2).setOffset(Offset + 4);
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Hi)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
    MBBI->getOperand(2).setOffset(Offset);
  } else {
    assert(isInt<12>(MBBI->getOperand(2).getImm() + 4));
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Hi)
                .addReg(MBBI->getOperand(1).getReg())
                .addImm(MBBI->getOperand(2).getImm() + 4);
  }

  // Order: Hi, Lo
  if (IsOp1EqualToLo) {
    MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Lo)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
  }

  MachineFunction *MF = MBB.getParent();
  SmallVector<MachineMemOperand *> NewLoMMOs;
  SmallVector<MachineMemOperand *> NewHiMMOs;
  for (const MachineMemOperand *MMO : MBBI->memoperands()) {
    NewLoMMOs.push_back(MF->getMachineMemOperand(MMO, 0, 4));
    NewHiMMOs.push_back(MF->getMachineMemOperand(MMO, 4, 4));
  }
  MIBLo.setMemRefs(NewLoMMOs);
  MIBHi.setMemRefs(NewHiMMOs);

  MBBI->eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandPseudoReadVLENBViaVSETVLIX0(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register Dst = MBBI->getOperand(0).getReg();
  unsigned Mul = MBBI->getOperand(1).getImm();
  RISCVVType::VLMUL VLMUL = RISCVVType::encodeLMUL(Mul, /*Fractional=*/false);
  unsigned VTypeImm = RISCVVType::encodeVTYPE(
      VLMUL, /*SEW=*/8, /*TailAgnostic=*/true, /*MaskAgnostic=*/true);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::PseudoVSETVLIX0))
      .addReg(Dst, RegState::Define)
      .addReg(RISCV::X0, RegState::Kill)
      .addImm(VTypeImm);

  MBBI->eraseFromParent();
  return true;
}

class RISCVPreRAExpandPseudo : public MachineFunctionPass {
public:
  const RISCVSubtarget *STI;
  const RISCVInstrInfo *TII;
  static char ID;

  RISCVPreRAExpandPseudo() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  StringRef getPassName() const override {
    return RISCV_PRERA_EXPAND_PSEUDO_NAME;
  }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAuipcInstPair(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI,
                           MachineBasicBlock::iterator &NextMBBI,
                           unsigned FlagsHi, unsigned SecondOpcode);
  bool expandLoadLocalAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadGlobalAddress(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI,
                               MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSIEAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSGDAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSDescAddress(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI,
                                MachineBasicBlock::iterator &NextMBBI);
  bool expandSLTIU(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   MachineBasicBlock::iterator &NextMBBI);
  bool expandSLTU(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandSRLI(MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandSRL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                 MachineBasicBlock::iterator &NextMBBI);
#ifndef NDEBUG
  unsigned getInstSizeInBytes(const MachineFunction &MF) const {
    unsigned Size = 0;
    for (auto &MBB : MF)
      for (auto &MI : MBB)
        Size += TII->getInstSizeInBytes(MI);
    return Size;
  }
#endif
};

char RISCVPreRAExpandPseudo::ID = 0;

bool RISCVPreRAExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  STI = &MF.getSubtarget<RISCVSubtarget>();
  TII = STI->getInstrInfo();

#if 0 //*PBH*: Check disabled
  const unsigned OldSize = getInstSizeInBytes(MF);
#endif

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);

#if 0 // *PBH*: Check disabled
  const unsigned NewSize = getInstSizeInBytes(MF);
  assert(OldSize >= NewSize);
#endif
  return Modified;
}

bool RISCVPreRAExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RISCVPreRAExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator MBBI,
                                      MachineBasicBlock::iterator &NextMBBI) {

  switch (MBBI->getOpcode()) {
  case RISCV::PseudoLLA:
    return expandLoadLocalAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLGA:
    return expandLoadGlobalAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_IE:
    return expandLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_GD:
    return expandLoadTLSGDAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLSDESC:
    return expandLoadTLSDescAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoSLTIU:
    return expandSLTIU(MBB, MBBI, NextMBBI);
  case RISCV::PseudoSLTU:
    return expandSLTU(MBB, MBBI, NextMBBI);
  case RISCV::SRLI:
    return expandSRLI(MBB, MBBI, NextMBBI);
  case RISCV::SRL:
    return expandSRL(MBB, MBBI, NextMBBI);
  }
  return false;
}

// PseudoSLTIU takes the same operands as the SLTIU instruction:
//
//   PseudoSLTIU rd, rs1, imm
//
bool RISCVPreRAExpandPseudo::expandSLTIU(
    MachineBasicBlock &OrigBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
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

  const auto &Subtarget = MF->getSubtarget<RISCVSubtarget>();
  if (!Subtarget.hasVendorXKeysomNoSltu()) {
    // Use "sltu" if we have it. The replacement is straightforward:
    //
    //      [... previous instrs ...]
    //      addi  Rs2, zero, imm
    //      sltu  rd, rs1, Rs2
    //      [... later instrs ...]
    Register Rs2 = MRI.createVirtualRegister(MRI.getRegClass(Rd));
    BuildMI(OrigBB, MBBI, DL, TII->get(RISCV::ADDI), Rs2)
        .addReg(Zero)
        .addImm(Imm);
    BuildMI(OrigBB, MBBI, DL, TII->get(RISCV::SLTU), Rd)
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
  BuildMI(OrigBB, OrigBB.end(), DL, TII->get(RISCV::ADDI), SubResult)
      .addReg(Rs1)
      .addImm(-Imm);
  Register ImmReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(OrigBB, OrigBB.end(), DL, TII->get(RISCV::ADDI), ImmReg)
      .addReg(Zero)
      .addImm(Imm);
  BuildMI(OrigBB, OrigBB.end(), DL, TII->get(RISCV::BGEU))
      .addReg(SubResult)
      .addReg(ImmReg)
      .addMBB(TrueBB);
  OrigBB.addSuccessor(TrueBB);
  OrigBB.addSuccessor(FalseBB);

  Register FalseReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*FalseBB, FalseBB->end(), DL, TII->get(RISCV::ADDI), FalseReg)
      .addReg(Zero)
      .addImm(0);
  BuildMI(*FalseBB, FalseBB->end(), DL, TII->get(RISCV::PseudoBR))
      .addMBB(PostBB);
  FalseBB->addSuccessor(PostBB);

  Register TrueReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*TrueBB, TrueBB->end(), DL, TII->get(RISCV::ADDI), TrueReg)
      .addReg(Zero)
      .addImm(1);
  // TrueBB falls through.
  TrueBB->addSuccessor(PostBB);

  // A phi node to def the final result.
  BuildMI(*PostBB, PostBB->begin(), DL, TII->get(TargetOpcode::PHI), Rd)
      .addReg(FalseReg)
      .addMBB(FalseBB)
      .addReg(TrueReg)
      .addMBB(TrueBB);

  NextMBBI = OrigBB.end();
  MI.eraseFromParent();

  return true;
}

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

bool RISCVPreRAExpandPseudo::expandSRLI(MachineBasicBlock &OrigBB,
                                        MachineBasicBlock::iterator MBBI,
                                        MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *const MF = OrigBB.getParent();
  if (!STI->hasVendorXKeysomNoSrli()) {
    return false;
  }

  static constexpr auto Zero = RISCV::X0;
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SRLI to have 3 operands");
  DebugLoc DL = MI.getDebugLoc();

  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  int64_t ShAmt = MI.getOperand(2).getImm();

  MachineRegisterInfo &MRI = MF->getRegInfo();
  InstructionHelper Helper{MRI, MRI.getRegClass(Rd), OrigBB,   MBBI,
                           DL,  this->STI,           this->TII};

  if (!STI->hasVendorXKeysomNoSrl()) {
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
  const auto Mask = (1U << (STI->getXLen() - ShAmt)) - 1U;
  if (Mask >= 1 << 12) {
    Register UpperImm = MRI.createVirtualRegister(MRI.getRegClass(Rd));
    // TODO: in theory, LUI is an instruction that can be disabled!
    BuildMI(OrigBB, MBBI, DL, TII->get(RISCV::LUI), UpperImm)
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
bool RISCVPreRAExpandPseudo::expandSRL(MachineBasicBlock &OrigBB,
                                       MachineBasicBlock::iterator MBBI,
                                       MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *const MF = OrigBB.getParent();
  if (!STI->hasVendorXKeysomNoSrl()) {
    return false;
  }

  static constexpr auto Zero = RISCV::X0;
  MachineInstr &MI = *MBBI;
  assert(MI.getNumOperands() == 3 && "Expected SRL to have 3 operands");
  DebugLoc DL = MI.getDebugLoc();

  Register Rd = MI.getOperand(0).getReg();
  Register Rs1 = MI.getOperand(1).getReg();
  Register Rs2 = MI.getOperand(2).getReg();

  MachineRegisterInfo &MRI = MF->getRegInfo();
  const TargetRegisterClass *const RegClass = MRI.getRegClass(Rd);

  InstructionHelper Helper{MRI, RegClass,  OrigBB,   MBBI,
                           DL,  this->STI, this->TII};

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
  auto XLenImm = Helper.rvAddi(Zero, STI->getXLen());
  auto Dist = Helper.rvSub(XLenImm, Rs2Bounded);
  // Create the inverted mask
  Register SextMaskInv = Helper.rvSll(AllOnes, Dist);
  // Invert to get the true mask
  Register SextMask = Helper.rvXori(SextMaskInv, -1);

  Helper.rvAnd(Rd, ShiftA, SextMask);

  MI.eraseFromParent();
  return true;
}

bool RISCVPreRAExpandPseudo::expandSLTU(MachineBasicBlock &OrigBB,
                                        MachineBasicBlock::iterator MBBI,
                                        MachineBasicBlock::iterator &NextMBBI) {

  // The replacement code should look like:
  //
  //  OrigBB:
  //      [... previous instrs ...]
  //      bltu  rs1, rs2, TrueBB
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
  assert(MI.getNumOperands() == 3 && "Expected PseudoSLTU to have 3 operands "
                                     "(matching the SLTU instruction)");
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

  BuildMI(OrigBB, OrigBB.end(), DL, TII->get(RISCV::BLTU))
      .addReg(Rs1)
      .addReg(Rs2)
      .addMBB(TrueBB);
  OrigBB.addSuccessor(TrueBB);
  OrigBB.addSuccessor(FalseBB);

  Register FalseReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*FalseBB, FalseBB->end(), DL, TII->get(RISCV::ADDI), FalseReg)
      .addReg(Zero)
      .addImm(0);
  BuildMI(*FalseBB, FalseBB->end(), DL, TII->get(RISCV::PseudoBR))
      .addMBB(PostBB);
  FalseBB->addSuccessor(PostBB);

  Register TrueReg = MRI.createVirtualRegister(MRI.getRegClass(Rd));
  BuildMI(*TrueBB, TrueBB->end(), DL, TII->get(RISCV::ADDI), TrueReg)
      .addReg(Zero)
      .addImm(1);
  // TrueBB falls through.
  TrueBB->addSuccessor(PostBB);

  // A phi node to def the final result.
  BuildMI(*PostBB, PostBB->begin(), DL, TII->get(TargetOpcode::PHI), Rd)
      .addReg(FalseReg)
      .addMBB(FalseBB)
      .addReg(TrueReg)
      .addMBB(TrueBB);

  NextMBBI = OrigBB.end();
  MI.eraseFromParent();

  return true;
}

bool RISCVPreRAExpandPseudo::expandAuipcInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);

  MachineOperand &Symbol = MI.getOperand(1);
  Symbol.setTargetFlags(FlagsHi);
  MCSymbol *AUIPCSymbol = MF->getContext().createNamedTempSymbol("pcrel_hi");

  MachineInstr *MIAUIPC =
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::AUIPC), ScratchReg).add(Symbol);
  MIAUIPC->setPreInstrSymbol(*MF, AUIPCSymbol);

  MachineInstr *SecondMI =
      BuildMI(MBB, MBBI, DL, TII->get(SecondOpcode), DestReg)
          .addReg(ScratchReg)
          .addSym(AUIPCSymbol, RISCVII::MO_PCREL_LO);

  if (MI.hasOneMemOperand())
    SecondMI->addMemOperand(*MF, *MI.memoperands_begin());

  MI.eraseFromParent();
  return true;
}

bool RISCVPreRAExpandPseudo::expandLoadLocalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI,
                             RISCV::ADDI);
}

bool RISCVPreRAExpandPseudo::expandLoadGlobalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  unsigned SecondOpcode = STI->is64Bit() ? RISCV::LD : RISCV::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_GOT_HI,
                             SecondOpcode);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  unsigned SecondOpcode = STI->is64Bit() ? RISCV::LD : RISCV::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GOT_HI,
                             SecondOpcode);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSGDAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GD_HI,
                             RISCV::ADDI);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSDescAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::LD : RISCV::LW;

  Register FinalReg = MI.getOperand(0).getReg();
  Register DestReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);

  MachineOperand &Symbol = MI.getOperand(1);
  Symbol.setTargetFlags(RISCVII::MO_TLSDESC_HI);
  MCSymbol *AUIPCSymbol = MF->getContext().createNamedTempSymbol("tlsdesc_hi");

  MachineInstr *MIAUIPC =
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::AUIPC), ScratchReg).add(Symbol);
  MIAUIPC->setPreInstrSymbol(*MF, AUIPCSymbol);

  BuildMI(MBB, MBBI, DL, TII->get(SecondOpcode), DestReg)
      .addReg(ScratchReg)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_LOAD_LO);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), RISCV::X10)
      .addReg(ScratchReg)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_ADD_LO);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::PseudoTLSDESCCall), RISCV::X5)
      .addReg(DestReg)
      .addImm(0)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_CALL);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADD), FinalReg)
      .addReg(RISCV::X10)
      .addReg(RISCV::X4);

  MI.eraseFromParent();
  return true;
}

} // end of anonymous namespace

INITIALIZE_PASS(RISCVExpandPseudo, "riscv-expand-pseudo",
                RISCV_EXPAND_PSEUDO_NAME, false, false)

INITIALIZE_PASS(RISCVPreRAExpandPseudo, "riscv-prera-expand-pseudo",
                RISCV_PRERA_EXPAND_PSEUDO_NAME, false, false)

namespace llvm {

FunctionPass *createRISCVExpandPseudoPass() { return new RISCVExpandPseudo(); }
FunctionPass *createRISCVPreRAExpandPseudoPass() { return new RISCVPreRAExpandPseudo(); }

} // end of namespace llvm
