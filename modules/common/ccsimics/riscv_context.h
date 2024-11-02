// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_RISCV_CONTEXT_H_
#define MODULES_COMMON_CCSIMICS_RISCV_CONTEXT_H_

#include "ccsimics/context.h"
#include "ccsimics/riscv_simics_connection.h"
#include "ccsimics/simics_connection.h"
#include "c3/crypto/cc_encoding.h"

class RiscvContext : public Context {
 public:
    RiscvContext(SimicsConnection *con, CCPointerEncodingBase *ptrenc)
        : Context(con, ptrenc) {}
    virtual ~RiscvContext() = default;

    /**
     * @brief No C3 configuration ISA for RISC-V
     */
    template <typename CtxTy> inline void enable() {}
};

#endif  // MODULES_COMMON_CCSIMICS_RISCV_CONTEXT_H_
