// -*-c++-*-
/* libpixie.h, for reading a pixie event */


#ifndef LIBPIXIE_EXPERIMENT_DEFINITION_H
#define LIBPIXIE_EXPERIMENT_DEFINITION_H


#include <unordered_map>
#include <set>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "traces.hh"

namespace PIXIE {
  class Experiment_Definition {
  public:    
    FILE *file;   //experimental definition, ASCII

    struct Channel {
      int crateID;
      int slotID;
      int channelNumber;
      bool qdcs;
      bool eraw;
      bool traces;
      std::string algName;
      std::string algFile;
      int algIndex;
      Trace::Algorithm *alg;
      std::string name;
      bool isTagger;
      bool operator<(const Channel &other) const {return channelNumber < other.channelNumber; }
      bool operator==(const Channel &other) const { return !name.compare(other.name); }
      Channel(int crID,
              int slID,
              int chN,
              bool e,
              bool q,
              bool t,
              std::string algN,
              std::string algF,
              int algI,
	      bool isTag) :
        crateID(crID),
        slotID(slID),
        channelNumber(chN),
        qdcs(q),
        eraw(e),
        traces(t),
        algName(algN),
        algFile(algF),
        algIndex(algI),
        alg(NULL),
	isTagger(isTag){};
    };

    struct Slot {
      int slotID;
      int crateID;
      int freq;
      std::unordered_map<int, Channel*> channelMap;

      Slot(int cID, int sID) : crateID(cID), slotID(sID) {};

      Channel *GetChannel(int channelNumber) const;
      int AddChannel(int channelNumber,
                     bool eraw=false,
                     bool qdcs=false,
                     bool traces=false,
                     std::string algName = "",
                     std::string algFile = "",
                     int algIndex = -1,
		     bool isTagger = false);
    };

    struct Crate {
      int crateID;
      std::unordered_map<int, Slot*> slotMap;
      Crate(int cID) : crateID(cID) {};

      Slot *GetSlot(int slotID) const;
      int AddSlot(int slotID, int frequency);
    };
    
  public:    
    std::unordered_map<int, Crate*> crateMap;
    std::vector<Channel*> detectors;
    std::vector<Channel*> taggers;

  public:
    Experiment_Definition() : file(NULL) {};
    int open(const std::string &path);
    int read();
    int close() { fclose(this->file); return 0; }
   
    int AddChannel(int crateID,
                   int slotID,
                   int channelNumber,
                   bool eraw=false,
                   bool qdcs=false,
                   bool traces=false,
                   std::string algName = "",
                   std::string algFile = "",
                   int algIndex = -1,
		   bool isTagger=false);
    int AddSlot(int crateID, int slotID, int frequency);
    int AddCrate(int crateID);
    
    Crate *GetCrate(int crateID) const;
    Slot *GetSlot(int crateID, int slotID) const;
    Channel *GetChannel(int crateID, int slotID, int channelNumber) const;

    int set_algorithms();
    
    int print();
    int print(std::ofstream &out);

  }; //Experiment_Definition

} // namespace PIXIE

#endif //LIBPIXIE_EXPERIMENT_DEFINITION_H
