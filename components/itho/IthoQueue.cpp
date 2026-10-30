#include "IthoQueue.h"

namespace esphome
{
  namespace itho
  {

    IthoQueue::IthoQueue()
    {

      ithoSpeed = 0;
      ithoOldSpeed = 0;
      fallBackSpeed = 42;

#ifdef USE_ARDUINO
      queueUpdater.attach_ms(
          QUEUE_UPDATE_MS, +[](IthoQueue *queueInstance)
                           { queueInstance->update_queue(); },
          this);
#else
      esp_timer_create_args_t timer_args = {};
      timer_args.callback = [](void *arg) {
        static_cast<IthoQueue *>(arg)->update_queue();
      };
      timer_args.arg = this;
      timer_args.name = "itho_queue";
      esp_timer_create(&timer_args, &queueUpdater);
      esp_timer_start_periodic(queueUpdater, QUEUE_UPDATE_MS * 1000);
#endif

      ithoSpeedUpdated = false;
    }

    IthoQueue::~IthoQueue()
    {
#ifdef USE_ARDUINO
      queueUpdater.detach();
#else
      if (queueUpdater) {
        esp_timer_stop(queueUpdater);
        esp_timer_delete(queueUpdater);
        queueUpdater = nullptr;
      }
#endif
    }

    bool IthoQueue::add2queue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ)
    {
      if (validVal == 0)
      {
        fallBackSpeed = speedVal;
        if (nonQ_cmd_clearsQ)
        {
          clear_queue();
        }
        return true;
      }
      else
      {

        validVal = validVal * 60 * 1000; // 100 = 1000/QUEUE_UPDATE_MS //-> minutes2milliseconds
        for (int i = 0; i < MAX_QUEUE; i++)
        {
          if (items[i].speed == -1)
          {
            items[i].speed = speedVal;
            items[i].valid = validVal;
            return true;
          }
        }
        std::sort(items, items + MAX_QUEUE, [](const Queue s1, const Queue s2)
                  { return (s1.speed > s2.speed); });
        if (items[MAX_QUEUE - 1].speed < speedVal)
        {
          items[MAX_QUEUE - 1].speed = speedVal;
          items[MAX_QUEUE - 1].valid = validVal;
          return true;
        }
        return false;
      }
    }

    void IthoQueue::update_queue()
    {
      for (int i = 0; i < MAX_QUEUE; i++)
      {
        if (items[i].valid > 0)
        {
          items[i].valid -= QUEUE_UPDATE_MS;
        }
        if (items[i].valid == 0)
        {
          items[i].speed = -1;
        }
      }
      std::sort(items, items + MAX_QUEUE, [](const Queue s1, const Queue s2)
                { return (s1.speed > s2.speed); });
      ithoOldSpeed = ithoSpeed;
      if (items[0].speed == -1)
      {
        firstQueueItem = true;
        ithoSpeed = fallBackSpeed;
      }
      else
      {
        if (firstQueueItem)
        {
          firstQueueItem = false;
          fallBackSpeed = ithoSpeed;
        }
        ithoSpeed = items[0].speed;
      }
      if (ithoOldSpeed != ithoSpeed)
      {
        ithoSpeedUpdated = true;
      }
    }

    void IthoQueue::clear_queue()
    {
      for (int i = 0; i < MAX_QUEUE; i++)
      {
        items[i].speed = -1;
        items[i].valid = 0;
      }
    }

    void IthoQueue::Queue::get(JsonObject obj, int index) const
    {
      obj["index"] = index;
      obj["speed"] = speed;
      obj["valid"] = valid;
    }
    void IthoQueue::get(JsonObject obj)
    {
      // Add "queue" object
      JsonArray q = obj["queue"].to<JsonArray>();
      // Add each queue item in the array
      for (int i = 0; i < MAX_QUEUE; i++)
      {
        items[i].get(q.add<JsonObject>(), i);
      }
    }

  }
}
