#ifndef _LIM_MODEL_LIM_MODEL_CLASS_H_
#define _LIM_MODEL_LIM_MODEL_CLASS_H_

#include "ccsimics/simics_module.h"

#define FOR_OPTIONS(op)                                                        \
    op(debug_on, "enable debug messages");                                     \
    op(break_on_decode_fault,                                                  \
       "halt simulation when a non-canonical address is decoded");             \
    op(disable_meta_check, "disable LIM tag and bound checks");                \
    op(break_on_exception, "stop simulation on some exception");               \
    op(trace_only, "disable skip over metadata. This is for tracing only");    \
    op(data_disp, "displace data overlapping "                                 \
                  "metadata rather than shifting it");

#define MODEL_NAME "lim_model"
#define MODEL_CONNECTION "lim_model_connection"
#define MODEL_DESCRIPTION "LIM"

class LimSimicsConnection;

class LimSimicsModule final
    : public SimicsModule<LimSimicsModule, LimSimicsConnection> {
 public:
    static constexpr const char *kModelName = MODEL_NAME;
    static constexpr const char *kModelConnection = MODEL_CONNECTION;
    static constexpr const char *kModelDescription = MODEL_DESCRIPTION;
    LimSimicsModule(conf_object_t *obj)
        : SimicsModule<LimSimicsModule, LimSimicsConnection>(obj) {}

    static void register_model_specific_attributes(conf_class_t *cl) {}
};

#endif  // _LIM_MODEL_LIM_MODEL_CLASS_H_
