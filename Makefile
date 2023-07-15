all: allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test queue_test timers_test timers_benchmark_1 timers_benchmark_2 ranked_map_test message_passing_tree_test exception_with_backtrace_test exception_top_proto_storage_test sharder_test argparser_test lock_free_allocator_test

allocator_test: allocator.o allocator_test.o
	g++-9 -o allocator_test allocator_test.o allocator.o -O3 -pedantic -Wall -Werror -mcx16 -latomic

allocator_benchmark_1: allocator_benchmark_1.o
	g++-9 -o allocator_benchmark_1 allocator_benchmark_1.o -O3 -pedantic -Wall -Werror

allocator_benchmark_2: allocator_benchmark_2.o allocator.o
	g++-9 -o allocator_benchmark_2 allocator_benchmark_2.o allocator.o -O3 -pedantic -Wall -Werror

allocator_benchmark_3: allocator_benchmark_3.o
	g++-9 -o allocator_benchmark_3 allocator_benchmark_3.o -O3 -pedantic -Wall -Werror

allocator_benchmark_4: allocator_benchmark_4.o allocator.o
	g++-9 -o allocator_benchmark_4 allocator_benchmark_4.o allocator.o -O3 -pedantic -Wall -Werror

allocator_test.o: allocator_test.cpp
	g++-9 allocator_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_1.o: allocator_benchmark_1.cpp
	g++-9 allocator_benchmark_1.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_2.o: allocator_benchmark_2.cpp
	g++-9 allocator_benchmark_2.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_3.o: allocator_benchmark_3.cpp
	g++-9 allocator_benchmark_3.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

allocator_benchmark_4.o: allocator_benchmark_4.cpp
	g++-9 allocator_benchmark_4.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed.lib: packed.h
	touch packed.lib

allocator.o: allocator.cpp allocator.h packed.lib
	g++-9 allocator.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test.o: packed_test.cpp packed.lib
	g++-9 packed_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

packed_test: packed_test.o
	g++-9 -o packed_test packed_test.o -O3 -pedantic -Wall -Werror

queue_test: queue.o queue_test.o
	g++-9 -o queue_test queue_test.o queue.o -mcx16 -pthread -latomic -pedantic -Wall

queue.o: queue.cpp queue.h
	g++-9 queue.cpp -g -c -std=c++1z

queue_test.o: queue_test.cpp
	g++-9 queue_test.cpp -g -c -std=c++1z

timers.o: timers.cpp timers.h
	g++-9 timers.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_test.o: timers_test.cpp
	g++-9 timers_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_test: timers_test.o timers.o
	g++-9 -o timers_test timers.o timers_test.o -O3 -pedantic -Wall -Werror

timers_benchmark_1.o: timers_benchmark_1.cpp
	g++-9 timers_benchmark_1.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_benchmark_1: timers_benchmark_1.o
	g++-9 -o timers_benchmark_1 timers_benchmark_1.o -O3 -pedantic -Wall -Werror

timers_benchmark_2.o: timers_benchmark_2.cpp
	g++-9 timers_benchmark_2.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

timers_benchmark_2: timers_benchmark_2.o timers.o
	g++-9 -o timers_benchmark_2 timers_benchmark_2.o timers.o -O3 -pedantic -Wall -Werror

exception_with_backtrace.o: exception_with_backtrace.cpp exception_with_backtrace.h
	g++-9 exception_with_backtrace.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

exception_with_backtrace_test.o: exception_with_backtrace_test.cpp
	g++-9 exception_with_backtrace_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

exception_with_backtrace_test: exception_with_backtrace_test.o exception_with_backtrace.o
	g++-9 -o exception_with_backtrace_test exception_with_backtrace_test.o exception_with_backtrace.o -O3 -pedantic -Wall -Werror -lunwind -lbacktrace -ldl

ranked_map.lib: ranked_map.h types.lib
	touch ranked_map.lib

ranked_map_test.o: ranked_map_test.cpp ranked_map.lib
	g++-9 ranked_map_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

ranked_map_test: ranked_map_test.o allocator.o
	g++-9 -o ranked_map_test ranked_map_test.o allocator.o -O3 -pedantic -Wall -Werror

types.lib: types.h allocator.o type_specifier.lib
	touch types.lib

message_passing_tree.lib: message_passing_tree.h types.lib type_specifier.lib queue.o
	touch message_passing_tree.lib

message_passing_tree_test.o: message_passing_tree_test.cpp message_passing_tree.lib
	g++-9 message_passing_tree_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

message_passing_tree_test: message_passing_tree_test.o allocator.o queue.o
	g++-9 -o message_passing_tree_test message_passing_tree_test.o allocator.o queue.o -O3 -pedantic -Wall -Werror -mcx16 -latomic

type_specifier.lib: type_specifier.h
	touch type_specifier.lib

exception_top_proto_storage.pb.h: exception_top_proto_storage.proto
	protoc -I=. --cpp_out=. exception_top_proto_storage.proto

exception_top_proto_storage.pb.o: exception_top_proto_storage.pb.h exception_top_proto_storage.pb.cc
	g++-9 exception_top_proto_storage.pb.cc -g -c -std=c++1z -O3 -pedantic -Wall -Werror

exception_top_proto_storage.o: exception_top_proto_storage.h exception_top_proto_storage.cpp exception_top_proto_storage.pb.o timers.o ranked_map.lib
	g++-9 exception_top_proto_storage.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

exception_top_proto_storage_test.o: exception_top_proto_storage_test.cpp exception_top_proto_storage.o exception_with_backtrace.o
	g++-9 exception_top_proto_storage_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

exception_top_proto_storage_test: exception_top_proto_storage_test.o exception_top_proto_storage.o exception_top_proto_storage.pb.o exception_with_backtrace.o timers.o allocator.o
	g++-9 -o exception_top_proto_storage_test exception_top_proto_storage_test.o exception_top_proto_storage.o exception_top_proto_storage.pb.o exception_with_backtrace.o timers.o allocator.o -O3 -pedantic -Wall -Werror -lstdc++fs -lprotobuf -lunwind -lbacktrace -ldl


sharder.lib: sharder.h types.lib timers.o message_passing_tree.lib exception_top_proto_storage.o
	touch sharder.lib

sharder_test.o: sharder_test.cpp sharder.lib message_passing_tree.lib
	g++-9 sharder_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

sharder_test: sharder_test.o allocator.o timers.o exception_top_proto_storage.o exception_top_proto_storage.pb.o
	g++-9 -o sharder_test sharder_test.o allocator.o timers.o exception_top_proto_storage.o exception_top_proto_storage.pb.o -O3 -pedantic -Wall -Werror -mcx16 -latomic -lpthread -lprotobuf

auto_registrar.lib: auto_registrar.h
	touch auto_registrar.lib

argparser.o: argparser.h argparser.cpp types.lib auto_registrar.lib
	g++-9 argparser.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

argparser_test.o: argparser_test.cpp argparser.o
	g++-9 argparser_test.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror

argparser_test: argparser.o argparser_test.o exception_with_backtrace.o auto_registrar.lib
	g++-9 -o argparser_test argparser.o argparser_test.o exception_with_backtrace.o allocator.o -O3 -pedantic -Wall -Werror -lbacktrace -ldl

lock_free_allocator.o: lock_free_allocator.cpp lock_free_allocator.h packed.lib
	g++-9 lock_free_allocator.cpp -g -c -std=c++1z -O3 -pedantic -Wall -Werror -fdiagnostics-color=always

lock_free_allocator_test: lock_free_allocator_test.cpp lock_free_allocator.o
	g++-9 -o lock_free_allocator_test lock_free_allocator.o -O3 -pedantic -Wall -Werror -lbacktrace -ldl -fdiagnostics-color=always

clean:
	rm -f *.o *.gch *.lib *.pb.cc *.pb.h allocator_test allocator_benchmark_1 allocator_benchmark_2 allocator_benchmark_3 allocator_benchmark_4 packed_test queue_test timers_test timers_benchmark_1 timers_benchmark_2 ranked_map_test message_passing_tree_test exception_with_backtrace_test exception_top_proto_storage_test sharder_test file\ 0 file\ 1 core argparser_test lock_free_allocator_test

