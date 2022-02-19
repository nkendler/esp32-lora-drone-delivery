#include <Arduino.h>

#include "lorabase.h"

#ifdef LORA_SENDER
#include "lorasender.h"
#endif
#ifdef LORA_RECEIVER
#include "lorareceiver.h"
#endif

ECE496::LoraBase *lora_module;

void setup() 
{
  #ifdef LORA_SENDER
  lora_module = new ECE496::LoraSender();
  #endif
  #ifdef LORA_RECEIVER
  lora_module = new ECE496::LoraReceiver();
  #endif

  lora_module->setup();
}

void loop()
{
  lora_module->loop();
}