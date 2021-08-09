#ifndef PIXIE2ROOT_HH
#define PIXIE2ROOT_HH

class options {
public:
  int events_per_read;
  bool live;
  size_t breakatevent;
  std::string path_output;
  std::string defPath;  //experimental definition (ASCII)
  std::string listPath;  //listmode data input file
  bool verbose;
  int coincWindow;
  int liveCount;
  bool timeOrder;
  int minMult;
  bool warnings;
  int nThreads;
  bool QDCs;
  bool rawE;
  bool traces;
public:
  options()
    : events_per_read(1000),
      live(false),
      breakatevent(0),
      path_output("pixie.root"),
      verbose(false),
      coincWindow(-1),
      liveCount(0),
      timeOrder(false),
      minMult(1),
      warnings(false),
      nThreads(1),
      QDCs(false),
      rawE(false),
      traces(false)
      
  { }
};

struct PixieEvent {
  ULong64_t eventTime;
  UInt_t eventRelTime;
  //UInt_t headerLength;
  //UInt_t eventLength;
  UInt_t finishCode;
  //UInt_t CFDTime;
  UInt_t CFDForce;
  UInt_t eventEnergy;
  UInt_t outOfRange;

  UInt_t ESumTrailing;
  UInt_t ESumLeading;
  UInt_t ESumGap;
  UInt_t baseline;  

  UInt_t QDCSums[8]={0};
  
public:
  PixieEvent() : eventTime(0),
                 eventRelTime(0),
                 finishCode(0),
                 CFDForce(0),
                 eventEnergy(0),
                 outOfRange(0),
                 ESumTrailing(0),
                 ESumLeading(0),
                 ESumGap(0),
                 baseline(0)
  {}
  void Reset() {
    eventTime = 0;
    eventRelTime = 0;
    finishCode = 0;
    CFDForce = 0;
    eventEnergy = 0;
    outOfRange = 0;

    ESumTrailing = 0;
    ESumLeading = 0;
    ESumGap = 0;
    baseline = 0;

    std::fill(QDCSums, QDCSums+8, 0);
  }
};

struct PixieTraceEvent {
  std::vector<Int_t> meas;
  PixieTraceEvent(int len) : meas(len) {}
  void Reset() {
    std::fill(meas.begin(), meas.end(), 0);
  }
};

struct PixieTagger {
  ULong64_t taggerTime;
  //UInt_t taggerRelTime;
  UInt_t taggerValue;
  UInt_t taggerNew;
public:
  PixieTagger() : taggerTime(0), taggerValue(0), taggerNew(0) {}
  void Reset() { taggerTime = 0; taggerValue = 0; taggerNew = 0; }
};

class PixieThread {
public:
  //TFile *file;
  PIXIE::Reader reader;
  PIXIE::Experiment_Definition definition;
  options opt;
  int threadNum;
  unsigned long long offset;
  unsigned long long max_offset;
  //PIXIE::Trace::Algorithm *tracealg;
  //PixieThread(TFile *f, PIXIE::Reader r, options op, int i, unsigned long long off) : file(f), reader(r), opt(op), threadNum(i), offset(off) {};

  PixieThread(options op, PIXIE::Experiment_Definition def, int i, unsigned long long off, unsigned long long moff) : opt(op), definition(def), threadNum(i), offset(off), max_offset(moff) {
  };  
};

namespace PIXIE {
  void *Process(void *thread);
}

#endif
