CXXFLAGS=-pthread
BINARIES=bin/barrier bin/latch
LIBS=libs/libconcurrencyTS.a

all: dirs $(LIBS) $(BINARIES)

.PHONY: dirs
dirs:
	mkdir -p bin libs objects

.PHONY: clean
clean:
	rm -rf *.o $(BINARIES) $(LIBS)

libs/libconcurrencyTS.a: barrier.o latch.o
	mkdir -p `dirname $@`
	ar rc $@ $^

bin/%: %.cc %.h
	$(CXX) -g -o $@ $< $(CXXFLAGS)
