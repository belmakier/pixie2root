/*
  pixie2root: converts an XIA Pixie-16 (time-sorted) listmode data file into a ROOT Tree
  Branches are named as crate.slot.channel, filled with 0 if nothing fired
  Takes a coincidence window as an input
*/

#include<iostream>
#include<vector>
#include<unordered_map>
#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cerrno>
#include<sstream>
#include<fstream>

#include<pthread.h>

/* EXTERN */
#include "args/args.hxx"

#include "TROOT.h"
#include "TFile.h"
#include "Compression.h"
#include "TTree.h"
#include "TH1D.h"

#include "pixie.hh"
#include "experiment_definition.hh"
#include "trace_algorithms.hh"
#include "pixie2root.hh"

static const struct PixieEvent EmptyChannel;

int main(int argc, char ** argv) {
  ROOT::EnableThreadSafety();
  options options;

  args::ArgumentParser parser("pixie2root conversion utility","Timothy Gray <timothy.gray@anu.edu.au");

  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Group filegroup(parser, "Required files", args::Group::Validators::All);
  args::ValueFlag<std::string> expdef(filegroup, "exptdef.expt", "Experimental definition file", {'d', "expdef"});
  args::ValueFlag<std::string> listmode(filegroup, "pixie_data.evt.to", "Listmode data file", {'i', "listmode"});
  args::ValueFlag<std::string> rootfile(filegroup, "test.root", "Output ROOT file", {'o', "rootfile"});
  
  args::Flag live(parser, "live", "Live mode", {'l', "live"});
  args::Flag timeorder(parser, "timeorder", "Time-order mode", {'t', "timeorder"});
  args::Flag warnings(parser, "warnings", "Display warnings", {'w', "warnings"});
  args::Flag verbose(parser, "verbose", "Verbose output", {'v', "verbose"});
  
  args::ValueFlag<ULong64_t> n_events_per_read(parser, "10000", "Events per read", {'n', "eventsperread"}, 10000);
  args::ValueFlag<UInt_t> coinc(parser, "20", "Coincidence window (in units of 10 ns)", {'c', "coincidence"}, 20);
  args::ValueFlag<UInt_t> mult(parser, "1", "Minimum multiplicy to write to Tree", {'m', "multiplicty"}, 1);
  args::ValueFlag<ULong64_t> n_events(parser, "0", "Events to process, zero = all", {'N', "nevents"}, 0);
  args::ValueFlag<UInt_t> n_threads(parser, "1", "Number of cores", {'j', "cores"}, 1);
  
  args::Flag qdcs(parser, "qdcs", "QDCs", {'q', "qdcs"});
  args::Flag eraw(parser, "eraw", "Raw Energy Sums", {'e', "eraw"});
  args::Flag traces(parser, "traces", "Traces", {'z', "traces"});

  try { parser.ParseCLI(argc, argv); }
  catch (args::Help) {
    std::cout << parser;
    return 0;
  }
  catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }
  catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 2;
  }

  options.live                     = args::get(live);
  options.events_per_read          = args::get(n_events_per_read);
  options.breakatevent             = args::get(n_events);
  options.verbose                  = args::get(verbose);
  options.warnings                 = args::get(warnings);
  options.timeOrder                = args::get(timeorder);
  options.minMult                  = args::get(mult);
  options.coincWindow              = args::get(coinc);
  options.nThreads                 = args::get(n_threads);
  options.QDCs                     = args::get(qdcs);
  options.rawE                     = args::get(eraw);
  options.traces                   = args::get(traces);

  options.defPath                  = args::get(expdef).c_str();
  options.listPath                 = args::get(listmode).c_str();
  options.path_output              = args::get(rootfile).c_str();

  if (options.timeOrder && options.live) {
    std::cout << "Can't have live mode and time-ordering: run pxi-time-order with -l in a separate shell" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (options.timeOrder) {
    std::string cmd = "Pixie16TimeOrder ";
    cmd.append(options.listPath);
    system(cmd.c_str());
    options.listPath.append(".to");
  }
  int nThreads = options.nThreads;

  // Finished parsing command line, start doing.
  // --------------------------------------------------
  time_t starttime;
  time_t endtime;

  printf("Sorting data from file: " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", options.listPath.c_str());  

  PIXIE::Experiment_Definition definition;
  int retval = definition.open(options.defPath);
  if (retval<0) { std::cout << "Definition file not opened successfully, retval = " << retval << std::endl; return(-1); }
  if (options.verbose==true) {
      printf("Reading definition from file " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", options.defPath.c_str());
  }
  definition.read();
  if (options.verbose==true ) {
    definition.print();
  }
  definition.close();

  PIXIE::PreReader prereader(nThreads);

  retval = prereader.open(options.listPath);
  if (retval < 0) {
    std::cout << "Error, could not open listmode file: " << options.listPath << " retval= " << retval << std::endl;
    return -1;
  }
  time(&starttime);
  std::cout << "Pre-reading to determine offsets for each thread" << std::endl;
  prereader.read(options.breakatevent);
  prereader.print();

  time(&endtime);
  time_t preread_time = endtime-starttime;
  std::cout << "Pre-reading time = " << preread_time << " s" << std::endl;

  pthread_t threads[nThreads];
  pthread_attr_t attr;
  void *status;
  int rc;
  std::vector<PixieThread*> pixie_threads;
  
  for (int i=0; i<nThreads; ++i) {
    off_t max_offset = -1;
    if (i < nThreads-1) { max_offset = prereader.offsets[i+1]; }
    //printf("%ld\n",max_offset);
    PixieThread* pixie_thread = new PixieThread(options, definition, i, prereader.offsets[i], max_offset);
    pixie_threads.push_back(pixie_thread);
  }

  time(&starttime);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i=0; i<nThreads; ++i) {
    //std::cout << "Creating thread " << i << std::endl;
    rc = pthread_create(&threads[i],&attr,PIXIE::Process,(void *)(pixie_threads[i]));
    if (rc) {
      std::cout << "Error: unable to create thread, " << rc << std::endl;
      exit(-1);
    }
  }

  pthread_attr_destroy(&attr);
  PIXIE::Reader reader;

  for (int i=0; i<nThreads; ++i) {
    rc = pthread_join(threads[i], &status);
    if (rc) {
      std::cout << "Error: unable to join thread, " << rc << std::endl;
      exit(-1);
    }
    reader.pileups += (pixie_threads[i]->reader).pileups;
    reader.badcfd += (pixie_threads[i]->reader).badcfd;
    reader.subevents += (pixie_threads[i]->reader).subevents;
    reader.nEvents += (pixie_threads[i]->reader).nEvents;
    reader.sameChanPU += (pixie_threads[i]->reader).sameChanPU;
    reader.outofrange += (pixie_threads[i]->reader).outofrange;
    reader.mults[0] += (pixie_threads[i]->reader).mults[0];
    reader.mults[1] += (pixie_threads[i]->reader).mults[1];
    reader.mults[2] += (pixie_threads[i]->reader).mults[2];
    reader.mults[3] += (pixie_threads[i]->reader).mults[3];
    std::cout << std::endl << "[ " << i << " ] Finished sorting " << std::endl;
  }
  
  time(&endtime);
  time_t filltime = endtime-starttime;
  
  // final print
  //--------------------------------------------
  std::cout << "Total fill time = " << filltime << " s " << std::endl;

  printf("\n");
  printf("Total sub-events:     " ANSI_COLOR_YELLOW "%15lld\n" ANSI_COLOR_RESET, reader.subevents);
  printf("Bad CFD sub-events:   " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", reader.badcfd, 100*(1-(double)reader.badcfd/(double)reader.subevents));
  printf("Pileups:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", reader.pileups, 100*(1-(double)reader.pileups/(double)reader.subevents));
  printf("Same channel pileups: " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", reader.sameChanPU, 100*(1-(double)reader.sameChanPU/(double)reader.subevents));
  printf("Out of range:         " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", reader.outofrange, 100*(1-(double)reader.outofrange/(double)reader.subevents));
  printf("\n");
  printf("Singles:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", reader.mults[0], 100*(double)reader.mults[0]/(double)reader.nEvents);
  printf("Doubles:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", reader.mults[1], 100*(double)reader.mults[1]/(double)reader.nEvents);
  printf("Triples:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", reader.mults[2], 100*(double)reader.mults[2]/(double)reader.nEvents);
  printf("Quadruples:           " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", reader.mults[3], 100*(double)reader.mults[3]/(double)reader.nEvents);

  if (nThreads==1) {
    std::rename((options.path_output+"_"+std::to_string(0)).c_str(), (options.path_output).c_str());
  }
 
  std::cout<<std::endl;
  pthread_exit(NULL);
} //main
