#ifndef __LORABASE_H__
#define __LORABASE_H__

namespace ECE496 
{
  class LoraBase 
  {
  public:
    virtual void setup();
    virtual void loop();
  };
}

#endif /*__LORABASE_H__*/