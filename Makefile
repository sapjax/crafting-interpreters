exec = a.out
CC= gcc
sources = ${wildcard src/*.c}
objects = ${sources:.c=.o}
flags = -g -Wall -fPIC 

${exec}: ${objects}
	${CC} ${flags} -o ${exec} ${objects}
 
 %.o: %.c include/%.h
	${CC} ${flags} -c $< -o $@
 
 clean:
	-rm *.out
	-rm *.o
	-rm src/*.o
	-rm *.a
 
 lint:
	clang-tidy src/*.c src/include/*.h
