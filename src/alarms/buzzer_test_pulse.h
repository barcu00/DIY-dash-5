#pragma once
#include <cstdint>
// Manual test is separate from real alarm cadence/acknowledgement.
class BuzzerTestPulse {
public:
    void request(uint32_t now) {since_=now;active_=true;}
    bool update(uint32_t now,bool enabled) {
        if(!enabled || now-since_>=200)active_=false;
        return active_;
    }
private:
    uint32_t since_=0;bool active_=false;
};
