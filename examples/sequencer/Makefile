all: build

test: build
	@dune exec ./sequencer.exe

c:
	gcc trying.c -lasound -o trying
	./trying

build:
	@dune build
