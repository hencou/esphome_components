#pragma once

#include "esphome/components/json/json_util.h"
#include <Ticker.h>

namespace esphome
{
  namespace itho
  {

#define MAX_QUEUE 10
#define QUEUE_UPDATE_MS 100

    class IthoQueue
    {

    public:

      IthoQueue();
      ~IthoQueue();
      
      bool add2queue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ);
      void clear_queue();
      uint16_t getIthoSpeed(){return ithoSpeed;};
      bool getIthoSpeedUpdated() {return ithoSpeedUpdated;}
      void setIthoSpeedUpdated(bool value) {ithoSpeedUpdated = value;}

      void get(JsonObject);
       
    private:
      void update_queue();
      void set_itho_fallback_speed(uint16_t speedVal) { fallBackSpeed = speedVal; };

      uint16_t ithoSpeed;
      uint16_t ithoOldSpeed;
      uint16_t fallBackSpeed;

      struct Queue
      {
        int16_t speed{-1};
        unsigned long valid{0};
        void get(JsonObject, int index) const;
      };

      bool ithoSpeedUpdated;
      mutable bool firstQueueItem{true};
      struct Queue items[MAX_QUEUE];
      Ticker queueUpdater;

    };

  }
}
