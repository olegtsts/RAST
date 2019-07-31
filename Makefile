all: allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test

allocator_test: allocator.o allocator_test.o
	g++-7 -o allocator_test allocator_test.o allocator.o -O3 -pedantic -Wall -Werror

allocator_benchmark_1: allocator_benchmark_1.o
	g++-7 -o allocator_benchmark_1 allocator_benchmark_1.o -O3 -pedantic -Wall -Werror

allocator_benchmark_2: allocator_benchmark_2.o allocator.o
	g++-7 -o allocator_benchmark_2 allocator_benchmark_2.o allocator.o -O3 -pedantic -Wall -Werror

allocator_benchmark_3: allocator_benchmark_3.o
	g++-7 -o allocator_benchmark_3 allocator_benchmark_3.o -O3 -pedantic -Wall -Werror

allocator_benchmark_4: allocator_benchmark_4.o allocator.o
	g++-7 -o allocator_benchmark_4 allocator_benchmark_4.o allocator.o -O3 -pedantic -Wall -Werror

allocator_test.o: allocator_test.cpp
	g++-7 allocator_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_1.o: allocator_benchmark_1.cpp
	g++-7 allocator_benchmark_1.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_2.o: allocator_benchmark_2.cpp
	g++-7 allocator_benchmark_2.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_3.o: allocator_benchmark_3.cpp
	g++-7 allocator_benchmark_3.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_4.o: allocator_benchmark_4.cpp
	g++-7 allocator_benchmark_4.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator.o: allocator.cpp allocator.h
	g++-7 allocator.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test.o: packed_test.cpp
	g++-7 packed_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test: packed_test.o
	g++-7 -o packed_test packed_test.o -O3 -pedantic -Wall -Werror

clean:
	rm -f *.o *.gch allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test
