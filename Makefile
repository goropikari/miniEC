.PHONY: build build-calibration
build: build-calibration

build-calibration:
	./docker_build.sh miniec:all
