#ifndef __LORASENDER_H__
#define __LORASENDER_H__

#include "lorabase.h"

namespace ECE496
{
  class LoraSender: public ECE496::LoraBase 
  {
  public:
    void setup();
    void loop();
  };
}

#endif /*__LORASENDER_H__*/