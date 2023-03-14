
# To add dependencies, you can do that in individual makefiles scripts by just
# adding new install_depencencies_ubuntu tartet like so:
#
# install_dependencies_ubuntu::
#     do_something_here...
#
# Note the double colon!
#

ifeq ($(shell lsb_release -si), Ubuntu)
	_DEPENDENCY_TARGET = install_dependencies_ubuntu
else
	_DEPENDENCY_TARGET = install_dependencies_unknown
endif

install_dependencies_ubuntu::
	$(info === Generic dependencies)
	sudo apt install -y \
    	bison \
    	curl \
    	flex \
    	git \
    	libatk1.0-dev \
    	libatk-bridge2.0-dev \
    	libelf-dev \
    	libgtk-3-dev \
    	python3-pip
	pip3 install \
		pytest-xdist


install_dependencies_unknown::
	$(info )
	$(info Cannot detect current environment or install depencencies in it. \
	You may need to manually install dependencies. To view the installation \
	commands that would have been executed on Ubuntu, you can run (where `-n` \
	specifies dry-run):)
	$(info )
	$(info > make -n install_dependencies_ubuntu)
	$(info )
	$(error Unknown OS: '$(shell lsb_release -si)')
	@false

PHONY: install_dependencies
install_dependencies: $(_DEPENDENCY_TARGET)
