.PHONY: build
build:
	./docker_build.sh miniec:all

.PHONY: fmt
fmt:
	dprint fmt
