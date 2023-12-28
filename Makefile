.PHONY: build
build:
	QMK_VERSION=0.23.2 ./docker_build.sh miniec
	QMK_VERSION=vial-qmk-20231210153557 ./docker_build.sh miniec:vial
