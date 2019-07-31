all: allocator_test packed_test

allocator_test: allocator.o allocator_test.o
	g++-7 -o allocator_test allocator_test.o allocator.o

allocator_test.o: allocator_test.cpp
	g++-7 allocator_test.cpp -g -c -std=c++1z

allocator.o: allocator.cpp allocator.h
	g++-7 allocator.cpp -g -c -std=c++1z

packed_test.o: packed_test.cpp
	g++-7 packed_test.cpp -g -c -std=c++1z

packed_test: packed_test.o
	g++-7 -o packed_test packed_test.o

clean:
	rm -f *.o *.gch allocator_test packed_test
