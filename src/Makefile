CXX=g++
CXXFLAGS=-Wall -Wextra -Wno-unused-result -pedantic -g -O3 -std=gnu++11 -pthread -msse2 -ftree-vectorize #-ftree-vectorizer-verbose=2
#CXXFLAGS=-Wall -Wextra -pedantic -g -O3 -std=gnu++11 -pthread -msse2 -ftree-vectorize #-ftree-vectorizer-verbose=2
LFLAGS=-lboost_filesystem -lboost_system #-Wl,--no-as-needed


flow: main.o params.o io.o ports.o ips.o
	$(CXX) $(CXXFLAGS) main.o params.o io.o ports.o ips.o -o $@ $(LFLAGS)

main.o: main.cpp params.h netflow.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o $@

params.o: params.cpp params.h
	$(CXX) $(CXXFLAGS) -c params.cpp -o $@

io.o: io.cpp io.h
	$(CXX) $(CXXFLAGS) -c io.cpp -o $@

ports.o: ports.cpp portdata.h flowdata.h params.h utils.h io.h
	$(CXX) $(CXXFLAGS) -c ports.cpp -o $@

ips.o: ips.cpp ipdata.h flowdata.h params.h utils.h io.h
	$(CXX) $(CXXFLAGS) -c ips.cpp -o $@


.PHONY: clean

clean-all:
	rm -rf *.o flow
clean:
	rm -rf *.o