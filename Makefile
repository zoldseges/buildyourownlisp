all: src/main.c src/rules.h
	cc src/main.c lib/mpc/mpc.c -std=c99 -Wall -Ilib -ledit -lm -o lispy

src/rules.h: src/rules.txt
	xxd -i $^ > src/rules.h

clean:
	rm src/rules.h
