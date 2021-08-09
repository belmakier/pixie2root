// -*-c++-*-
/* libpixie reader */

#ifndef LIBPIXIE_READER_H
#define LIBPIXIE_READER_H

#include <stdio.h>
#include <sys/stat.h>

#include "event.hh"
#include "traces.hh"

namespace PIXIE {
  class Reader {
  public:
    //bool binary;
    Experiment_Definition definition;
    int liveSort; //start at end of file?
    long long pileups;
    long long badcfd;
    long long subevents;
    long long nEvents;
    long long sameChanPU;
    long long outofrange;
    long long mults[4];
    
    FILE *file;
    
    PIXIE::Trace::Algorithm *tracealg;

    off_t fileLength;
    time_t starttime;
    size_t eventsread;

    off_t max_offset;
    off_t start_offset;

    int thread;
    
    bool end;
    
  public:
    Reader();
    ~Reader();

    const Experiment_Definition & experiment_definition() const
    {
      return (definition);
    };

    bool eof();
    off_t offset() const;
    off_t set_offset(off_t s_offset);
    off_t update_filesize();
    bool check_pos();
    void printUpdate();
    
    int set_algorithm(PIXIE::Trace::Algorithm *&alg);

    int open(const std::string &path);
    int read(std::vector<Event> &events,
             int               coincWindow,
             int               max,
             off_t             max_offset,
             bool              warnings);
    int dump_traces(int crate, int slot, int chan, std::string outPath, int maxTraces, int append, std::string traceName);

    void start() {
      eventsread = 0;
      time(&starttime);
    }
    
  };
  
} // namespace PIXIE
      

#endif //LIBPIXIE_READER_H
