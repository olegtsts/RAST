queue_test: queue.o queue_test.o
	g++-7 -o queue_test queue_test.o queue.o -mcx16 -pthread -latomic -pedantic -Wall

queue.o: queue.cpp queue.h
	g++-7 queue.cpp -g -c -std=c++1z

queue_test.o: queue_test.cpp
	g++-7 queue_test.cpp -g -c -std=c++1z

clean:
	rm -f *.o *.gch queue_test
