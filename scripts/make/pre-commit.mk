
install_dependencies_ubuntu::
	$(info === Dependencies for pre-commit hook)
	sudo apt install -y \
		clang-format \
		clang-tidy
	pip3 install \
		cpplint \
		pre-commit

pre-commit-install:
	pre-commit install

pre-commit-uninstall:
	pre-commit uninstall
