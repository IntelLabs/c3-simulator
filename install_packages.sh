#!/bin/sh

# Need to do `-f config-user.mk` here since we may not have set up the Simics
# Makefiles at this point.
make -f config-user.mk install_dependencies
