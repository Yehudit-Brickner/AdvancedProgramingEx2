# .PHONY: all
# all: task stdinExample

# task:	codec.h basic_main.c
# 	gcc basic_main.c -L. -l Codec -o encoder

# stdinExample:	stdin_main.c
# 		gcc stdin_main.c -L. -l Codec -o tester

# .PHONY: clean
# clean:
# 	-rm encoder tester 2>/dev/null


.PHONY: all
all: task stdinExample threadpool1 threadpool2

task:	codec.h basic_main.c
	gcc basic_main.c -L. -l:libCodec.so -o encoder

export: 
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

stdinExample:	stdin_main.c
		gcc stdin_main.c -L. -l:libCodec.so -o tester

threadpool1: codec.h chatthread.c
	gcc -pthread chatthread.c -L. -l:libCodec.so -o tpool

threadpool2: codec.h OurThreadPool.c
	gcc -pthread OurThreadPool.c -L. -l:libCodec.so -o tp

.PHONY: clean
clean:
	-rm encoder tester tpool tp
