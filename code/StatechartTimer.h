/* --- StatechartTimer.h --- */
#ifndef STATECHARTTIMER_H
#define STATECHARTTIMER_H

#include "./src-gen/Statechart.h"
#include <Ticker.h> 

class StatechartTimer : public sc::timer::TimerServiceInterface {
public:
  StatechartTimer() {
    instance = this;
  }
  void setTimer(sc::timer::TimedInterface* sm, sc_eventid event, sc_time time_ms, sc_boolean isPeriodic) override {
    // store context
    this->sm = sm;
    this->eventId = event;
    // detach any previous
    ticker.detach();
    // use static callback
    if (isPeriodic) {
      ticker.attach_ms((uint32_t)time_ms, onTimeout);
    } else {
      ticker.once_ms((uint32_t)time_ms, onTimeout);
    }
  }
  void unsetTimer(sc::timer::TimedInterface* /*sm*/, sc_eventid /*event*/) override {
    ticker.detach();
  }
private:
  static void onTimeout() {
    if (instance && instance->sm) {
      instance->sm->raiseTimeEvent(instance->eventId);
    }
  }
  static StatechartTimer* instance;
  sc::timer::TimedInterface* sm = nullptr;
  sc_eventid eventId = 0;
  Ticker ticker;
};

// define static instance pointer
StatechartTimer* StatechartTimer::instance = nullptr;

#endif // STATECHARTTIMER_H