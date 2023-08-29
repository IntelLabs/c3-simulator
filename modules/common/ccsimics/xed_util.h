/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_XED_UTIL_H_
#define MODULES_COMMON_CCSIMICS_XED_UTIL_H_

#include <simics/arch/x86.h>
extern "C" {
#include <xed-interface.h>  // NOTE! The xed header is C-only
}

static constexpr const int kDumpBufLen = 2048;

static inline int convert_xed_reg_to_simics(xed_reg_enum_t xed_reg) {
    switch (xed_reg) {
    case XED_REG_RIP:
        return X86_Reg_Id_PC;
    case XED_REG_RAX:
        return X86_Reg_Id_Rax;
    case XED_REG_RBX:
        return X86_Reg_Id_Rbx;
    case XED_REG_RCX:
        return X86_Reg_Id_Rcx;
    case XED_REG_RDX:
        return X86_Reg_Id_Rdx;
    case XED_REG_RSP:
        return X86_Reg_Id_Rsp;
    case XED_REG_RBP:
        return X86_Reg_Id_Rbp;
    case XED_REG_RSI:
        return X86_Reg_Id_Rsi;
    case XED_REG_RDI:
        return X86_Reg_Id_Rdi;
    case XED_REG_R8:
        return X86_Reg_Id_R8;
    case XED_REG_R9:
        return X86_Reg_Id_R9;
    case XED_REG_R10:
        return X86_Reg_Id_R10;
    case XED_REG_R11:
        return X86_Reg_Id_R11;
    case XED_REG_R12:
        return X86_Reg_Id_R12;
    case XED_REG_R13:
        return X86_Reg_Id_R13;
    case XED_REG_R14:
        return X86_Reg_Id_R14;
    case XED_REG_R15:
        return X86_Reg_Id_R15;
    default:
        return X86_Reg_Id_Not_Used;
    }
}

static inline void dump_xedd(const xed_decoded_inst_t *decoded_inst) {
    char buf[kDumpBufLen];
    xed_decoded_inst_dump(decoded_inst, buf, kDumpBufLen);
    dbgprint("%.*s", kDumpBufLen, buf);
}

#endif  // MODULES_COMMON_CCSIMICS_XED_UTIL_H_
