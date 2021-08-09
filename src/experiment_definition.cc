/* libpixie experimental definition implementation */

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "experiment_definition.hh"
#include "trace_algorithms.hh"

namespace PIXIE
{
  int Experiment_Definition::open(const std::string &path) {
    if (this->file) {
      return (-1); //file has already been opened
    }

    if (!(this->file = fopen(path.c_str(), "r"))) {
      return (-2);
    }

    return (0);
  }

  
  int Experiment_Definition::read() {
    if (!this->file) {
      return (-1);
    }

    std::stringstream ss;
    char cline[2048];

    int progress = 0;

    while(std::fgets(cline, sizeof cline, this->file)!=NULL) {
      std::string line (cline);
      switch (line[0]){
      case ' ':
      case '\t':
      case '\n':
      case '#':
        break;
      case 'D':
      case 'T':
        {
          ss.clear();
          ss.str(line);

          char flag;
          int crateID;
          int slotID;
          int channelNumber;
          int frequency;
          
          int eraw;
          int qdcs;
          int traces;

          std::string alg_name;
          std::string alg_file;
          int alg_index = -1;
          ss >> flag;
          ss >> crateID;
          ss >> slotID;
          ss >> channelNumber;
          ss >> frequency;
          if (ss >> eraw) { }
          else { eraw = false; }
          if (ss >> qdcs) { }
          else { qdcs = false; }        
          if (ss >> traces) { }
          else { traces = false; }

          if (ss >> alg_name) { }
          else { alg_name = ""; }
          if (ss >> alg_file) { }
          else { alg_file = ""; }
          if (ss >> alg_index) { }
          else { alg_index = -1; }
          
          this->AddCrate(crateID);
          this->AddSlot(crateID, slotID, frequency);
          
          std::string name = "c"+std::to_string(crateID)+"s"+std::to_string(slotID)+"ch"+std::to_string(channelNumber);
          if (flag=='D'){
	    if (this->AddChannel(crateID, slotID, channelNumber, eraw, qdcs, traces, alg_name, alg_file, alg_index,false)!=0) {
	      std::cout << "Caution: channel " << crateID << "." << slotID << "." << channelNumber << " defined twice, second definition ignored" << std::endl;
	      break;
	    }	    
            this->detectors.push_back(this->GetChannel(crateID, slotID, channelNumber));
          }
          else if (flag=='T'){
	    if (this->AddChannel(crateID, slotID, channelNumber, eraw, qdcs, traces, alg_name, alg_file, alg_index,true)!=0) {
	      std::cout << "Caution: channel " << crateID << "." << slotID << "." << channelNumber << " defined twice, second definition ignored" << std::endl;
	      break;
	    }	    
            this->taggers.push_back(this->GetChannel(crateID, slotID, channelNumber));
          }
          break;
        }
      case 'C':
        break;
      }
    }

    //create algorithm objects
    std::cout << "Setting algorithms" << std::endl;
    int nalgs = set_algorithms();
    std::cout << nalgs << " set" << std::endl;
    
    return (0);
  }//read_definition  
  

  Experiment_Definition::Slot * Experiment_Definition::Crate::GetSlot(int slotID) const {
    const auto slot = this->slotMap.find(slotID);

    if (slot != this->slotMap.end())
      return (slot->second);

    return (nullptr);
  }

  int Experiment_Definition::Crate::AddSlot(int slotID, int frequency) {
    slotMap[slotID] = new Slot(crateID, slotID);
    slotMap[slotID]->freq = frequency;

    return(0);
  }
  
  Experiment_Definition::Channel * Experiment_Definition::Slot::GetChannel(int channelNumber) const {
    const auto channel = this->channelMap.find(channelNumber);

    if (channel != this->channelMap.end())
      return (channel->second);

    return (nullptr);
  }

  int Experiment_Definition::Slot::AddChannel(int channelNumber,
                                              bool eraw,
                                              bool qdcs,
                                              bool traces,
                                              std::string algName,
                                              std::string algFile,
                                              int algIndex,
					      bool isTagger) {
    channelMap[channelNumber] = new Channel(crateID,
                                            slotID,
                                            channelNumber,
                                            eraw,
                                            qdcs,
                                            traces,
                                            algName,
                                            algFile,
                                            algIndex,
					    isTagger);

    return(0);
  }
  
  Experiment_Definition::Crate * Experiment_Definition::GetCrate(int crateID) const
  {//Gets crate with ID crateID, if it doesn't exist then returns null
    const auto crate = this->crateMap.find(crateID);

    if (crate != this->crateMap.end())
      return (crate->second);

    return (nullptr);
  }

  Experiment_Definition::Slot * Experiment_Definition::GetSlot(int crateID, int slotID) const
  {//Gets slot with ID slotID in crate with crateID, if it doesn't exist then returns null
    Experiment_Definition::Crate *crate = GetCrate(crateID);
    if (!crate) {
      return nullptr;
    }
    
    const auto slot = crate->GetSlot(slotID);

    return (slot);
  }

  Experiment_Definition::Channel * Experiment_Definition::GetChannel(int crateID, int slotID, int channelNumber) const
  {//Gets channel in slot with ID slotID in crate with crateID, if it doesn't exist then returns null
    Experiment_Definition::Slot *slot = GetSlot(crateID, slotID);
    if (!slot) {
      return nullptr;
    }
    
    const auto channel = slot->GetChannel(channelNumber);
    
    return (channel);
  }

  int Experiment_Definition::AddCrate(int crateID)
  {
    if (GetCrate(crateID)){
      return (-1);  //crate already exists
    }

    crateMap[crateID] = new Crate(crateID);

    return (0);
  }  //Add crate

  int Experiment_Definition::AddSlot(int crateID, int slotID, int frequency)
  {
    if (GetSlot(crateID, slotID))
      {
        return (-1);  //slot already exists 
      }

    //slot does not exist in crate
    auto crate = GetCrate(crateID);

    if (!crate)
      {
        //crate does not exist!
        return (-1);
      }

    //crate exists and slot doesn't

    crate->AddSlot(slotID, frequency);

    return (0);
  } //Add slot

  int Experiment_Definition::AddChannel(int crateID,
                                        int slotID,
                                        int channelNumber,
                                        bool eraw,
                                        bool qdcs,
                                        bool traces,
                                        std::string algName,
                                        std::string algFile,
                                        int algIndex,
					bool isTagger)
  {
    if (GetChannel(crateID, slotID, channelNumber))
      {
        return (-1);  //channel already exists
      }

    //channel does not exist
    auto slot = GetSlot(crateID, slotID);
    
    if (!slot) {
      //slot does not exist!
      return (-1);
    }
    //both crate and slot exist
    slot->AddChannel(channelNumber, eraw, qdcs, traces, algName, algFile, algIndex, isTagger);

    return (0);
  } //Add channel  

  int Experiment_Definition::print(std::ofstream &log)
  {
    for (auto &crate_it : crateMap ) {
      auto crate = crate_it.second;
      log << "Crate ID " << crate->crateID << std::endl;
      for ( auto &slot_it : crate->slotMap ) {
        auto slot = slot_it.second;
        log << "  Slot ID " << slot->slotID << "     Frequency " << slot->freq << " MHz" << std::endl;
        for (auto channel_it : slot->channelMap) {
          auto channel = channel_it.second;
          log << "    Channel " << channel->channelNumber;
          if (std::find(taggers.begin(), taggers.end(), channel)!=taggers.end())
            log << "    (Tagger)";
                
          log << std::endl;
        }
      }
    }
    return 0;
  }

  int Experiment_Definition::print()
  {
    for (auto &crate_it : crateMap ) {
      auto crate = crate_it.second;
      std::cout << "Crate ID " << crate->crateID << std::endl;
      for ( auto &slot_it : crate->slotMap ) {
        auto slot = slot_it.second;
        std::cout << "  Slot ID " << slot->slotID << "     Frequency " << slot->freq << " MHz" << std::endl;
        for (auto channel_it : slot->channelMap) {
          auto channel = channel_it.second;
          std::cout << "    Channel " << channel->channelNumber;
          if (channel->qdcs) { std::cout << "   QDCs"; }
          if (channel->eraw) { std::cout << "   Raw Energy Sums"; }
          if (channel->traces) { std::cout << "   Traces, (" << channel->algName << " \"" << channel->algFile << "\", ID="<<channel->algIndex<<")"; }
          if (std::find(taggers.begin(), taggers.end(), channel)!=taggers.end())
            std::cout << "    (Tagger)";
                
          std::cout << std::endl;
        }
      }
    }
    return 0; 
  }

  int Experiment_Definition::set_algorithms() {
    int n_chans = 0;
    for (const auto &crate_it : crateMap) {
      auto crate = crate_it.second;
      for (const auto &slot_it : crate->slotMap) {
        auto slot = slot_it.second;
        for (const auto &channel_it : slot->channelMap) {
          auto channel = channel_it.second;
          if (channel->traces) {
            ++n_chans;
            int retval = PIXIE::setTraceAlg(channel->alg, channel->algName, channel->algFile, channel->algIndex);
            if (retval < 0) {
              std::cout << "Trace Algorithm not set properly!" << std::endl;
            }
          }
        }
      }
    }
    return n_chans;
  }
    
}//PIXIE
