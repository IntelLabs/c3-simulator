/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#include <simics/simulator-api.h>
#include "lim_model.h"
#include "lim_connection.h"

#include <stdio.h>

FORCE_INLINE lim_model_t *
obj_to_lim_model(conf_object_t *obj)
{
        return (lim_model_t *)obj;
}

#define ENABLE(option, desc)                         \
        tool-> option = true;

static conf_object_t *
alloc_object(void *arg)
{
        lim_model_t *tool = MM_ZALLOC(1, lim_model_t);
        return &tool->obj;
}

static connection_t *
new_connection(lim_model_t *tt, conf_object_t *cpu, attr_value_t attr)
{
        strbuf_t sb = SB_INIT;
        sb_addfmt(&sb, "%s_%d", SIM_object_name(&tt->obj),
                  tt->next_connection_number);
        conf_object_t *c = SIM_create_object(connection_class, sb_str(&sb),
                                             attr);
        sb_free(&sb);
        
        if (!c)
                return NULL;

        tt->next_connection_number++;
        connection_t *con = obj_to_con(c);
        con->cpu = cpu;
        con->tracer = tt;
        
        con->pi_iface = SIM_C_GET_INTERFACE(cpu, processor_info_v2);
        con->ci_iface = SIM_C_GET_INTERFACE(cpu, cpu_instrumentation_subscribe);
        con->x86_aq_iface = SIM_C_GET_INTERFACE(cpu, x86_address_query);
        con->iq_iface = SIM_C_GET_INTERFACE(cpu, cpu_instruction_query);
        con->mq_iface = SIM_C_GET_INTERFACE(cpu, cpu_memory_query);
        con->eq_iface = SIM_C_GET_INTERFACE(cpu, cpu_exception_query);        
        con->x86_mq_iface = SIM_C_GET_INTERFACE(cpu, x86_memory_query);
        con->x86_ex_iface = SIM_C_GET_INTERFACE(cpu, x86_exception);
               
        con->pa_digits = (con->pi_iface->get_physical_address_width(cpu)+3) >> 2;
        con->va_digits = (con->pi_iface->get_logical_address_width(cpu)+3) >> 2;
        sb_init(&con->last_line);

        con->id = SIM_get_processor_number(cpu);
        return con;
}

static conf_object_t *
it_connect(conf_object_t *obj, conf_object_t *cpu, attr_value_t attr)
{
        lim_model_t *tt = obj_to_lim_model(obj);
        connection_t *con = new_connection(tt, cpu, attr);

        if (!con)
                return NULL;
        
        VADD(tt->connections, &con->obj);
        configure_connection(con);
        return &con->obj;
}

static void
it_disconnect(conf_object_t *obj, conf_object_t *con_obj)
{
        lim_model_t *tool = obj_to_lim_model(obj);
        VREMOVE_FIRST_MATCH(tool->connections, con_obj);
        SIM_delete_object(con_obj);
}


static int
it_delete_instance(conf_object_t *obj)
{
        lim_model_t *tool = obj_to_lim_model(obj);
        ASSERT_FMT(VEMPTY(tool->connections),
                   "%s deleted with active connections", SIM_object_name(obj));
        MM_FREE(obj);
        return 0;
}

static attr_value_t
get_file(void *param, conf_object_t *obj, attr_value_t *idx)
{
        lim_model_t *tool = obj_to_lim_model(obj);
        return SIM_make_attr_string(tool->file);
}

static set_error_t
set_file(void *param, conf_object_t *obj, attr_value_t *val,
           attr_value_t *idx)
{
        lim_model_t *tool = obj_to_lim_model(obj);
        if (SIM_attr_is_nil(*val)) {
                if (tool->fh) {
                        fclose(tool->fh);
                        tool->fh = NULL;
                }
                MM_FREE(tool->file);
                tool->file = NULL;
        } else {
                tool->file = MM_STRDUP(SIM_attr_string(*val));
                tool->fh = fopen(tool->file, "w");
                if (!tool->fh) {
                        printf("Cannot open file");
                        return Sim_Set_Illegal_Value;
                }
        }
                
        return Sim_Set_Ok;
}

void
init_local()
{
        static const class_data_t funcs = {
                .alloc_object = alloc_object,
                .delete_instance = it_delete_instance,                
                .description = "LIM",
                .kind = Sim_Class_Kind_Session
        };
        conf_class_t *cl = SIM_register_class("lim_model", &funcs);

        static const instrumentation_tool_interface_t it = {
                .connect = it_connect,
                .disconnect = it_disconnect,
        };
        SIM_REGISTER_INTERFACE(cl, instrumentation_tool, &it);

        SIM_register_typed_attribute(
                cl, "file",
                get_file, NULL,
                set_file, NULL,
                Sim_Attr_Optional,
                "s|n", NULL,
                "Output file");

        init_connection();
}


