#include <cstring>

#include "measurement.hh"
#include "traces.hh"

namespace PIXIE {
  
  Mask Measurement::mChannelNumber    = Mask(0xF, 0);
  Mask Measurement::mSlotID           = Mask(0xF0, 4);
  Mask Measurement::mCrateID          = Mask(0xF00, 8);
  Mask Measurement::mHeaderLength     = Mask(0x1F000, 12);
  Mask Measurement::mEventLength      = Mask(0x7FFE0000, 17);
  Mask Measurement::mFinishCode       = Mask(0x80000000, 31);
  Mask Measurement::mTimeLow          = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mTimeHigh         = Mask(0xFFFF, 0);
  Mask Measurement::mEventEnergy      = Mask(0xFFFF, 0);
  Mask Measurement::mTraceLength      = Mask(0x7FFF0000, 16);
  Mask Measurement::mTraceOutRange    = Mask(0x80000000, 31);
  
  Mask Measurement::mCFDTime100       = Mask(0x7FFF0000, 16);
  Mask Measurement::mCFDTime250       = Mask(0x3FFF0000, 16);
  Mask Measurement::mCFDTime500       = Mask(0x1FFF0000, 16);
  Mask Measurement::mCFDForce100      = Mask(0x80000000, 31);
  Mask Measurement::mCFDForce250      = Mask(0x80000000, 31);
  Mask Measurement::mCFDTrigSource250 = Mask(0x40000000, 30);
  Mask Measurement::mCFDTrigSource500 = Mask(0xE0000000, 29);

  Mask Measurement::mESumTrailing     = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mESumLeading      = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mESumGap          = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mBaseline         = Mask(0xFFFFFFFF, 0);
  
  Mask Measurement::mQDCSums          = Mask(0xFFFFFFFF, 0);

  CFD Measurement::ProcessCFD(unsigned int data, int frequency) {
    CFD retval = {};
    if (frequency == 100) {
      int CFDTime  = mCFDTime100(data);
      unsigned int CFDForce = mCFDForce100(data);
      retval = {CFDTime, CFDForce};
      return retval;
    }
    else if (frequency == 250) {
      int CFDTime       = mCFDTime250(data);
      unsigned int CFDTrigSource = mCFDTrigSource250(data);
      unsigned int CFDForce      = mCFDForce250(data);
      retval.CFDForce = CFDForce;
      if (CFDForce==0) {                  
        retval.CFDFraction = CFDTime - 16384*static_cast<int>(CFDTrigSource);
      }
      else {
        retval.CFDFraction = 16384;
      }      
    }
    else if (frequency == 500) {
      int CFDTime       = mCFDTime500(data);
      unsigned int CFDTrigSource = mCFDTrigSource500(data);
      if (CFDTrigSource == 7) {
        retval.CFDForce = 1;
        retval.CFDFraction = 40960*4/5;
      }
      else {
        retval.CFDForce = 0;
        retval.CFDFraction = (CFDTime + 8192*(static_cast<int>(CFDTrigSource)-1))*4/5;
      }
    }
    else {
      std::cout << "\n" ANSI_COLOR_RED "Waring! Slot has frequency of neither 100 MHz, 250 MHz, or 500 MHz" ANSI_COLOR_RESET<< std::endl;
    }
    return retval;
  }

  EventTime Measurement::ProcessTime(unsigned long long timestamp, unsigned int cfddat, int frequency) {    
    unsigned long long time = timestamp << 15;
    CFD cfd = ProcessCFD(cfddat, frequency);
    time += cfd.CFDFraction;
    if (frequency == 250) {
      time = time*8/10;
    }
    EventTime retval = {time, cfd.CFDForce};
    return retval;    
  }

  int Measurement::read(FILE *fpr, Experiment_Definition &definition, uint16_t *outTrace) {
    uint32_t firstWord;
    fpos_t pos;
    fgetpos(fpr, &pos);
    if (fread(&firstWord, (size_t) 4, (size_t) 1, fpr) != 1) {
      fsetpos(fpr, &pos);
      return -1;    
    }

    channelNumber = mChannelNumber(firstWord);
    slotID        = mSlotID(firstWord);
    crateID       = mCrateID(firstWord);
    headerLength  = mHeaderLength(firstWord);
    eventLength   = mEventLength(firstWord);
    finishCode    = mFinishCode(firstWord);

    int oldChan = channelNumber;
	 
    //read the rest of the header
    uint32_t otherWords[headerLength-1];
    if (fread(&otherWords, (size_t) 4, (size_t) headerLength-1, fpr) != (size_t) headerLength-1) {
      fsetpos(fpr, &pos);
      return -1;
    }
    
    uint32_t timestampLow  = mTimeLow(otherWords[0]);
    uint32_t timestampHigh = mTimeHigh(otherWords[1]);
   
    uint64_t timestamp=static_cast<uint64_t>(timestampHigh);
    timestamp=timestamp<<32;
    timestamp=timestamp+timestampLow;
       
    if (definition.GetChannel(crateID, slotID, channelNumber)) {
      EventTime time = ProcessTime(timestamp, otherWords[1], definition.GetSlot(crateID, slotID)->freq);
      eventTime = time.time;
      CFDForce = time.CFDForce;            
    }
    else {
      std::cout << "\nWarning! CrateID.SlotID.ChannelNumber " << crateID << "." << slotID << "." << channelNumber << " not found" << std::endl;
    }
    eventEnergy=mEventEnergy(otherWords[2]);
    traceLength=mTraceLength(otherWords[2]);
    outOfRange=mTraceOutRange(otherWords[2]);
    
    //Read the rest of the header
    if (headerLength==8) {//Raw energy sums
      ESumTrailing = mESumTrailing(otherWords[3]);
      ESumLeading  = mESumLeading(otherWords[4]);
      ESumGap      = mESumGap(otherWords[5]);
      baseline     = mBaseline(otherWords[6]);
    }
    else if (headerLength==12) {//QDCSums
      for (int i=0; i<8; i++) {
        QDCSums[i]=mQDCSums(otherWords[3+i]);
      }
    }
    else if (headerLength==16) { //Both
      ESumTrailing = mESumTrailing(otherWords[3]);
      ESumLeading  = mESumLeading(otherWords[4]);
      ESumGap      = mESumGap(otherWords[5]);
      baseline     = mBaseline(otherWords[6]);
        
      for (int i=0; i<8; i++) {
          QDCSums[i]=mQDCSums(otherWords[7+i]);
      }
    }
    
    //skip or proces the trace if recorded
    auto channel = definition.GetChannel(crateID, slotID, channelNumber);
    //no trace, do nothing
    if ((eventLength - headerLength) == 0) {;}
    //skip trace if not needed
    else if (outTrace==NULL && !channel->traces){
      if (fseek(fpr, (eventLength-headerLength)*4, SEEK_CUR)) {
	fsetpos(fpr, &pos);
	return -1;
      }
    }
    //Read trace
    else{
      uint16_t trace[traceLength]; // = {0};
      if (fread(&trace, (size_t) 2, (size_t) traceLength, fpr) != (size_t) traceLength) {
        fsetpos(fpr, &pos);
        return -1;
      }
      if (outTrace!=NULL){
	memcpy(outTrace, &trace,traceLength*sizeof(uint16_t));
      }
      if (channel->traces) {
        PIXIE::Trace::Algorithm *tracealg = channel->alg;
        if (!tracealg || !tracealg->loaded) {
          //no trace algorigthm, should never happen
        }
        else {
          auto tmeas = tracealg->Process(trace, traceLength);
          good_trace = tracealg->good_trace;
          for (auto &m : tmeas) {
            trace_meas.push_back(m);
          }
        }
      }
    }
    return 0;     
  }
    
  int Measurement::print() const
  {
    std::cout << "               Crate ID: " << crateID << std::endl;
    std::cout << "                Slot ID: " << slotID << std::endl;
    std::cout << "         Channel Number: " << channelNumber << std::endl;
    std::cout << "            Finish Code: " << finishCode << std::endl;
    std::cout << "             Event Time: " << eventTime << std::endl;
    std::cout << "          Event RelTime: " << eventRelTime << std::endl;    
    std::cout << "              CFD Force: " << CFDForce << std::endl;
    std::cout << "           Event Energy: " << eventEnergy << std::endl;
    std::cout << "           Out of Range: " << outOfRange << std::endl;
    
    if (headerLength==8 || headerLength == 16) {
      std::cout << "   Energy Sum (trailing): " << ESumTrailing << std::endl;
      std::cout << "    Energy Sum (leading): " << ESumLeading << std::endl;
      std::cout << "        Energy Sum (gap): " << ESumGap << std::endl;
      std::cout << "                Baseline: " << baseline << std::endl;
    }
    
    if (headerLength==12 || headerLength == 16) {
      std::cout << "                QDCSum0: " << QDCSums[0] << std::endl;
      std::cout << "                QDCSum1: " << QDCSums[1] << std::endl;
      std::cout << "                QDCSum2: " << QDCSums[2] << std::endl;
      std::cout << "                QDCSum3: " << QDCSums[3] << std::endl;
      std::cout << "                QDCSum4: " << QDCSums[4] << std::endl;
      std::cout << "                QDCSum5: " << QDCSums[5] << std::endl;
      std::cout << "                QDCSum6: " << QDCSums[6] << std::endl;
      std::cout << "                QDCSum7: " << QDCSums[7] << std::endl;
    } 
    return(0);
  }  
}
