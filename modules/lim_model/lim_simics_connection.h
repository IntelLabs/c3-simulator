/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#pragma once

#include <memory>
#include <simics/base/conf-object.h>
#include "ccsimics/simics_connection.h"
#include "lim_simics_module.h"

class lim_class;

class LimSimicsConnection : public SimicsConnection {
    using ConnectionTy = LimSimicsConnection;

 public:
    FOR_OPTIONS(DECLARE)

    std::unique_ptr<lim_class> class_model;

 public:
    LimSimicsConnection(conf_object_t *con) : SimicsConnection(con) {}
    virtual ~LimSimicsConnection() = default;

    virtual void configure();

    static inline void
    register_connection_specific_attributes(conf_class_t *cl) {
        auto connection_class = cl;
        FOR_OPTIONS(ATTR_REGISTER)
    }
};
