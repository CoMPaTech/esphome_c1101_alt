#include "esphome.h"
#include "IthoCC1101.h"
#include "Ticker.h"

IthoCC1101 rf;
void ITHOinterrupt() ICACHE_RAM_ATTR;
void ITHOcheck();

// extra for interrupt handling
bool ITHOhasPacket = false;
Ticker ITHOticker;
String State="Low"; // after startup it is assumed that the fan is running low
String OldState="Low";
int LastIDindex = 0;
int OldLastIDindex = 0;
long LastPublish=0; 
bool InitRunned = false;


class FanOutput : public Component, public FloatOutput {
  public:
    void write_state(float state) override {
      if (state < 0.4) {
        // low speed
        rf.sendCommand(IthoLow);
      } else if (state < 0.7) {
        // medium speed
        rf.sendCommand(IthoMedium);
      } else {
        // high speed
        rf.sendCommand(IthoHigh);
      }
    }
};

class FanRecv : public PollingComponent {
  public:

    // Publish two sensors
    // Speed: the speed the fan is running at (depending on your model 1-2-3 or 1-2-3-4
    TextSensor *fanspeed = new TextSensor();

    // For now poll every 15 seconds
    FanRecv() : PollingComponent(15000) { }

    void setup() {
      rf.init();
      // Followin wiring schema, change PIN if you wire differently
      pinMode(D1, INPUT);
      attachInterrupt(D1, ITHOinterrupt, RISING);
      //attachInterrupt(D1, ITHOcheck, RISING);
      rf.initReceive();
    }

    void update() override {
        fanspeed->publish_state(State.c_str());
    }


};

// Figure out how to do multiple switches instead of duplicating them
// we need
// send: low, medium, high, full
//       timer 1 (10 minutes), 2 (20), 3 (30)
// To optimize testing, reset published state immediately so you can retrigger (i.e. momentarily button press)
class FanSendIthoJoin : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
        rf.sendCommand(IthoJoin);
        State = "Join";
        publish_state(!state);
      }
    }
};

void ITHOinterrupt() {
	ITHOticker.once_ms(10, ITHOcheck);
}

void ITHOcheck() {
  noInterrupts();
  if (rf.checkForNewPacket()) {
    IthoCommand cmd = rf.getLastCommand();
    switch (cmd) {
      case IthoUnknown:
        ESP_LOGD("custom", "Unknown state");
        break;
      case IthoLow:
        ESP_LOGD("custom", "IthoLow");
        State = "Low";
        break;
      case IthoMedium:
        ESP_LOGD("custom", "Medium");
        State = "Medium";
        break;
      case IthoHigh:
        ESP_LOGD("custom", "High");
        State = "High";
        break;
      case IthoFull:
        ESP_LOGD("custom", "Full");
        State = "Full";
        break;
      case IthoJoin:
        break;
      case IthoLeave:
        break;
    }
  }
  interrupts();
}
