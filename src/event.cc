/* event implementation for libpixie */

#include <vector>
#include <iostream>

#include "experiment_definition.hh"
#include "event.hh"
#include "colors.hh"

namespace PIXIE
{
  int Event::read(FILE *fpr,
                  Experiment_Definition &definition,
                  int coincWindow,
                  off_t max_offset,
                  bool warnings) {

    fpos_t pos;
    Measurement meas;
    int retval = meas.read(fpr, definition);
    if (retval == -1) {
      return 1; //end of file return
    }
    
    AddMeasurement(meas);
    int lastCrate = meas.crateID;
    int lastSlot = meas.slotID;
    int lastChan = meas.channelNumber;
    
    uint64_t maxTime = meas.eventTime + coincWindow;
    uint64_t triggerTime = meas.eventTime;

    int mult = 1;

    int curEvent = 1;
    
    while (curEvent) {
      //loop for current event
      fgetpos(fpr, &pos);          

      if (max_offset > 0 && ftello(fpr) >= max_offset) {
        std::cout << max_offset << "   " << ftello(fpr) << std::endl;
        retval = 1; //end of file return
        break;
      }
             
      Measurement next_meas;
      retval = next_meas.read(fpr, definition);
      if (retval == -1) {
        retval = 1;  //end of file
        break;
      }
             
      //   get event time, check if it's in the coincidence window
      if (next_meas.eventTime<=maxTime) {
        //in current event
        //check for duplicate in same channel
        if (GetMeasurement(next_meas.crateID, next_meas.slotID, next_meas.channelNumber)) {
          const Measurement *dup_meas = GetMeasurement(next_meas.crateID, next_meas.slotID, next_meas.channelNumber);
	  
          if (warnings) {
            std::cout << ANSI_COLOR_RED << "Warning: Channel " << dup_meas->slotID << ":" << dup_meas->channelNumber << " already fired in this event, perhaps the coincidence window is too large?" ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_RED << dup_meas->eventTime << " vs " << next_meas.eventTime << " :  " << (double)(-dup_meas->eventTime + next_meas.eventTime)/3276.8 << ANSI_COLOR_RESET << std::endl;
          }
          //if it's not a tagger, end this event and start another
	  auto *channel = definition.GetChannel(next_meas.crateID, next_meas.slotID, next_meas.channelNumber);
	  if(!channel->isTagger){
	    curEvent = 0;
	    retval = 2; //end of event with same channel pileup
	    break;
	  }
        }
                
        mult = mult + 1;
		 
        if (next_meas.eventTime<maxTime-coincWindow) {
          std::cout<< ANSI_COLOR_RED "\nWarning! File is not properly time-sorted" << std::endl;
          std::cout<< "First event: "
		   << lastCrate << "." 
		   << lastSlot << "." 
		   << lastChan << "   " << maxTime - coincWindow << std::endl;
	  std::cout<< "Second event: "
		   << next_meas.crateID << "."
		   << next_meas.slotID << "."
		   << next_meas.channelNumber << "   " << next_meas.eventTime << ANSI_COLOR_RESET << std::endl;

        }
                
        AddMeasurement(next_meas);
	lastCrate = next_meas.crateID;
	lastSlot = next_meas.slotID;
	lastChan = next_meas.channelNumber;
        maxTime = next_meas.eventTime+coincWindow;
        //go to next sub-event
      }
      else {
        curEvent=0; //breaks current event loop
      }
    }//loop for current event

    // increment multiplicity count
    if ( mult < 5 ) {
      this->mults[mult-1] = this->mults[mult-1] + 1;
    } 
    //set read position back to start of current sub-event
    fsetpos(fpr, &pos);

    return 0;
  }

  int Event::print()
  {
    std::cout << "======= NEW EVENT ======" << std::endl;
    for (const Measurement &meas : fMeasurements) {
      meas.print();
    }
    return (0);
  }
  
}//PIXIE namespace
                       
