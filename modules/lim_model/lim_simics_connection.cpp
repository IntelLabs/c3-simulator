#include "lim_simics_connection.h"
#include <simics/util/help-macros.h>
#include "model.h"

void LimSimicsConnection::configure() {
    class_model = std::make_unique<lim_class>(this);
    class_model->register_callbacks(this);

    if (this->debug_on) {
        SIM_printf("DEBUG messages enabled\n");
    }

    class_model->custom_model_init(this);
}
