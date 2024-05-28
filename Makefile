.PHONY: build
build:
	QMK_VERSION=0.24.11 ./docker_build.sh miniec
	QMK_VERSION=vial-latest ./docker_build.sh miniec:vial
