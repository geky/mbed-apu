
// NES APU Emulator
//

#ifndef APU_H
#define APU_H

#include <stdint.h>
#include "mbed-drivers/mbed.h"

namespace apu {


// APU Settings
#define APU_FREQ 894886
#define APU_SCALE 8


// Base channel representation
class Channel {
protected:
    int _timer;
    int _tick;

    uint16_t _period;
    uint8_t _duty;
    uint8_t _volume;

public:
    // Channel lifetime
    Channel();
    virtual ~Channel() = default;

    // Emulate a channel
    virtual void update(int cycles);

    // Trigger a channel update
    virtual void tick() = 0;

    // Get the current amplitude of the channel
    virtual uint8_t output() = 0;

    // Enable/disable specified channel
    virtual void enable();
    virtual void disable();

    // Set the note being played by a channel
    // Note value starts at A0
    virtual void note(uint8_t note);

    // Set the volume of a channel
    virtual void volume(uint8_t volume);

    // Set the duty cycle of a channel
    virtual void duty(uint8_t duty);
};

// NES Channels
class Square : public Channel {
public:
    virtual void tick();
    virtual uint8_t output();
};

class Triangle : public Channel {
public:
    virtual void update(int);
    virtual void tick();
    virtual uint8_t output();
};

class Noise : public Channel {
private:
    uint16_t _shift;

public:
    virtual void note(uint8_t);
    virtual void tick();
    virtual uint8_t output();
};


// Audio processing unit
class APU {
private:
    Channel **_channels;
    unsigned _count;
    uint8_t _output;

    Ticker _ticker;
    AnalogOut _dac;

    void update();

public:
    // APU lifetime
    APU(Channel **channels, unsigned count, PinName pin = DAC0_OUT);

    // Starting/stopping the unit
    void start();
    void stop();

    // Enable/disable specified channel
    void enable(unsigned channel);
    void disable(unsigned channel);

    // Set the note being played by a channel
    // Note value starts at A0
    void note(unsigned channel, uint8_t note);

    // Set the volume of a channel
    void volume(unsigned channel, uint8_t volume);

    // Set the duty cycle of a channel
    void duty(unsigned channel, uint8_t duty);

    // Get the current 6-bit amplitude of the APU
    uint8_t output();
};


}

#endif

