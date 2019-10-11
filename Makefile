CXXFLAGS=-pthread
BINARIES=bin/barrier bin/latch bin/future

all: dirs $(BINARIES)

.PHONY: dirs
dirs:
	mkdir -p bin libs objects

.PHONY: clean
clean:
	rm -rf *.o $(BINARIES)

bin/%: %.cc %.h
	$(CXX) -std=c++14 -o $@ $< $(CXXFLAGS)
