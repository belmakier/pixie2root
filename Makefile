#Makefile for pixie2root
SHELL := /bin/bash

FLAGS = -std=c++17 -I`root-config --incdir` -I./extern -O3
ROOTFLAGS = -L`root-config --libdir` `root-config --glibs --new `
LDFLAGS= $(ROOTFLAGS) -L./lib -lpixie -lpthread
COMPILER=clang++

all : bin/pixie2root bin/pixie_mcas bin/pixie_scalers bin/basic_test bin/pixie_dumpTraces

obj : 
	mkdir -p obj

bin : 
	mkdir -p bin

lib : 
	mkdir -p lib

bin/pixie2root : obj/pixie2root.o obj/process.o lib/libpixie.so | obj bin lib
	$(COMPILER) $(FLAGS) -o bin/pixie2root obj/pixie2root.o obj/process.o $(LDFLAGS)

obj/pixie2root.o : src/pixie2root.cc | obj
	$(COMPILER) $(FLAGS) -c -o obj/pixie2root.o src/pixie2root.cc

lib/libpixie.so : obj/measurement.o obj/event.o obj/reader.o obj/experiment_definition.o obj/pre_reader.o obj/trace_algorithms.o src/pixie.hh src/pre_reader.hh src/traces.hh src/trace_algorithms.hh | obj lib
	$(COMPILER) $(FLAGS) -shared -o lib/libpixie.so obj/measurement.o obj/event.o obj/reader.o obj/experiment_definition.o obj/pre_reader.o obj/trace_algorithms.o $(ROOTFLAGS)

obj/measurement.o : src/measurement.cc src/measurement.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/measurement.o src/measurement.cc

obj/event.o : src/event.cc src/event.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/event.o src/event.cc

obj/reader.o : src/reader.cc src/reader.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/reader.o src/reader.cc

obj/pre_reader.o : src/pre_reader.cc src/pre_reader.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/pre_reader.o src/pre_reader.cc

obj/trace_algorithms.o : src/trace_algorithms.cc src/traces.hh src/trace_algorithms.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/trace_algorithms.o src/trace_algorithms.cc

obj/experiment_definition.o : src/experiment_definition.cc src/experiment_definition.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/experiment_definition.o src/experiment_definition.cc

obj/process.o : src/process.cc src/pixie2root.hh | obj
	$(COMPILER) $(FLAGS) -fPIC -c -o obj/process.o src/process.cc

bin/pixie_dumpTraces: src/pixie_dumpTraces.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/pixie_dumpTraces src/pixie_dumpTraces.cc $(LDFLAGS)

bin/pixie_mcas: src/pixie_mcas.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/pixie_mcas src/pixie_mcas.cc $(LDFLAGS)

bin/pixie_scalers: src/pixie_scalers.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/pixie_scalers src/pixie_scalers.cc $(LDFLAGS)

bin/pixie_diagnostics: src/pixie_diagnostics.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/pixie_diagnostics src/pixie_diagnostics.cc $(LDFLAGS)

bin/pixie_diagnostics_basic: src/pixie_diagnostics_basic.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/pixie_diagnostics_basic src/pixie_diagnostics_basic.cc $(LDFLAGS)

bin/basic_test: src/basic_test.cc | bin
	$(COMPILER) $(FLAGS) -fPIC -o bin/basic_test src/basic_test.cc $(LDFLAGS)

install : bin/pixie2root lib/libpixie.so bin/pixie_dumpTraces
	cp bin/* $(HOME)/.local/bin;	
	cp lib/* $(HOME)/.local/lib;
	mkdir -p $(HOME)/.local/include/pixie2root;
	cp src/*.hh $(HOME)/.local/include/pixie2root/;

clean :
	rm -rf obj/

cleanall :
	rm -rf obj/;
	rm -rf bin/;
	rm -rf lib/;
