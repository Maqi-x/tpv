all: build-release

build-release:
	@./build.sh build --mode release

build-debug:
	@./build.sh build --mode debug

rebuild-release:
	@./build.sh rebuild --mode release

rebuild-debug:
	@./build.sh rebuild --mode debug

clean:
	@./build.sh clean

run:
	@./build.sh run

rebuild-run:
	@./build.sh rebuild-run

install:
	@./build.sh install --mode release
