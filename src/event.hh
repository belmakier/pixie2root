// -*-c++-*-
/* libpixie event */

#ifndef LIBPIXIE_EVENT_H
#define LIBPIXIE_EVENT_H

#include <vector>

#include "experiment_definition.hh"
#include "measurement.hh"
#include "traces.hh"

namespace PIXIE {
  class Event {
  public:
    std::vector<Measurement> fMeasurements;

    long long pileups;
    long long badcfd;
    long long outofrange;
    long long mults[4];

  public:
    Event() :  //initialise counters to zero
      pileups(0),
      badcfd(0),
      outofrange(0)
    {
      mults[0] = 0;
      mults[1] = 0;
      mults[2] = 0;
      mults[3] = 0;      
    }
    int print();
    int AddMeasurement(Measurement meas) {
      fMeasurements.push_back(meas);
      //increment counters
      if (meas.finishCode == 1) {
        ++pileups;
      }

      badcfd += meas.CFDForce;
      outofrange += meas.outOfRange;
      
      return 0;
    }
    int read(FILE *fpr,
             Experiment_Definition &definition,
             int coincWindow,
             off_t max_offset,
             bool warnings);
    
    const Measurement *GetMeasurement(int crateID, int slotID, int channelNumber) const
    {
      for (const Measurement &meas : fMeasurements) {
        if (meas.crateID == crateID &&
            meas.slotID == slotID &&
            meas.channelNumber == channelNumber) {
          return (&meas);
        }
      }
      return (NULL);
    }
  };
  
} // namespace PIXIE

#endif //LIBPIXIE_EVENT_H
