all: allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test message_passing_tree_test

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

packed.lib: packed.h
	touch packed.lib

allocator.o: allocator.cpp allocator.h packed.lib
	g++-7 allocator.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test.o: packed_test.cpp packed.lib
	g++-7 packed_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test: packed_test.o
	g++-7 -o packed_test packed_test.o -O3 -pedantic -Wall -Werror

types.lib: types.h allocator.o
	touch types.lib

message_passing_tree.lib: message_passing_tree.h types.lib
	touch message_passing_tree.lib

message_passing_tree_test.o: message_passing_tree_test.cpp message_passing_tree.lib
	g++-7 message_passing_tree_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

message_passing_tree_test: message_passing_tree_test.o allocator.o
	g++-7 -o message_passing_tree_test message_passing_tree_test.o allocator.o -O3 -pedantic -Wall -Werror

clean:
	rm -f *.o *.gch *.lib allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test message_passing_tree_test
