all: allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test queue_test timers_test timers_benchmark_1 timers_benchmark_2

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

queue_test: queue.o queue_test.o
	g++-7 -o queue_test queue_test.o queue.o -mcx16 -pthread -latomic -pedantic -Wall

queue.o: queue.cpp queue.h
	g++-7 queue.cpp -g -c -std=c++1z

queue_test.o: queue_test.cpp
	g++-7 queue_test.cpp -g -c -std=c++1z

timers.o: timers.cpp timers.h
	g++-7 timers.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_test.o: timers_test.cpp
	g++-7 timers_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_test: timers_test.o timers.o
	g++-7 -o timers_test timers.o timers_test.o -O3 -pedantic -Wall -Werror

timers_benchmark_1.o: timers_benchmark_1.cpp
	g++-7 timers_benchmark_1.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_benchmark_1: timers_benchmark_1.o
	g++-7 -o timers_benchmark_1 timers_benchmark_1.o -O3 -pedantic -Wall -Werror

timers_benchmark_2.o: timers_benchmark_2.cpp
	g++-7 timers_benchmark_2.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_benchmark_2: timers_benchmark_2.o timers.o
	g++-7 -o timers_benchmark_2 timers_benchmark_2.o timers.o -O3 -pedantic -Wall -Werror

clean:
	rm -f *.o *.gch allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test queue_test timers_test timers_benchmark_1 timers_benchmark_2