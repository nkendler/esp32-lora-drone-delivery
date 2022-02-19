#ifndef __LORARECEIVER_H__
#define __LORARECEIVER_H__

#include "lorabase.h"

namespace ECE496
{
  class LoraReciever: public ECE496::LoraBase 
  {
  public:
    void setup();
    void loop();
  };
}

#endif /*__LORARECEIVER_H__*/