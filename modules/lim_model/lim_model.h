/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef INSTRUMENTATION_TRACER_TOOL_H
#define INSTRUMENTATION_TRACER_TOOL_H

#include <simics/base/types.h>
#include <simics/base/conf-object.h>
#include <simics/simulator/conf-object.h>
#include <simics/util/alloc.h>
#include <simics/util/hashtab.h>

#include <simics/simulator-iface/instrumentation-tool.h>
#include <simics/simulator/output.h>

#include <simics/base/memory-transaction.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct lim_model {
        conf_object_t obj;
        char *file;
        FILE *fh;
        unsigned cpu_id;
        int next_connection_number;
        VECT(conf_object_t *) connections;
} lim_model_t;

#if defined(__cplusplus)
}
#endif        
#endif
