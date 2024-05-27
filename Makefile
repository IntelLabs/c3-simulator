#
# NOTE: The simics installed GNUMakefile will override this.
#
#       This only ensure that we can run initial setup or Docker-only builds
#       without needing the Simics GNUMakeifle or other project setup done by
#       Simics project-setup.
#

default_target: simics_setup

include config-user.mk
