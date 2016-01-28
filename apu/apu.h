
// NES APU Emulator
//

#ifndef APU_H
#define APU_H

#include <stdint.h>
#include "mbed-drivers/mbed.h"

namespace apu {


// APU Settings
#define APU_FREQ 1789772
#define APU_MAX 64

class APU; // predeclared


// Base channel representation
class Channel {
protected:
    friend APU;

    unsigned _tick;
    uint16_t _output;

    uint16_t _period;
    bool _update;

    uint8_t _duty;
    uint8_t _volume;
    int16_t _pitch;

    Ticker _ticker;
    APU *_apu;

    void retick(unsigned);

public:
    // Channel lifetime
    Channel();
    virtual ~Channel() = default;

    // Trigger a channel update
    virtual void tick();
    virtual void update() = 0;

    // Enable/disable specified channel
    virtual void enable();
    virtual void disable();

    // Set the note being played by a channel
    // Note value starts at A0
    virtual void note(uint8_t note);

    // Find the mapping of a period to a note
    virtual uint16_t to_period(uint8_t note) = 0;

    // Sets the period being played by the channel directly
    // Period is based on NES clock cycles
    virtual void set_period(uint16_t period);

    // Adjust period without timer reset
    virtual void adjust_period(uint16_t period);

    // Get the current period
    virtual uint16_t get_period();

    // Set the volume of a channel
    virtual void volume(uint8_t volume);

    // Set the pitch offset of a channel
    virtual void pitch(int16_t offset);

    // Set the duty cycle of a channel
    virtual void duty(uint8_t duty);

    // Get the current amplitude of the channel
    virtual uint8_t output();
};

// NES Channels
class Square : public Channel {
public:
    virtual uint16_t to_period(uint8_t);
    virtual void update();
};

class Triangle : public Channel {
public:
    virtual uint16_t to_period(uint8_t);
    virtual void update();
};

class Noise : public Channel {
private:
    uint16_t _shift = 0x0001;

public:
    virtual uint16_t to_period(uint8_t);
    virtual void update();
};


// Audio processing unit
class APU {
private:
    Channel **_channels;
    unsigned _count;
    uint8_t _output;

    AnalogOut _dac;

public:
    // APU lifetime
    APU(Channel **channels, unsigned count, PinName pin=DAC0_OUT);

    // Enable/disable specified channel
    void enable(unsigned channel);
    void disable(unsigned channel);

    // Set the note being played by a channel
    // Note value starts at A0
    void note(unsigned channel, uint8_t note);

    // Sets the period being played by the channel directly
    // Period is based on NES clock cycles
    void period(unsigned channel, uint16_t period);

    // Set the volume of a channel
    void volume(unsigned channel, uint8_t volume);

    // Set the pitch offset of a channel
    void pitch(unsigned channel, int16_t offset);

    // Set the duty cycle of a channel
    void duty(unsigned channel, uint8_t duty);

    // Updates the output
    void update();

    // Get the current amplitude of the APU
    uint8_t output();
};


}

#endif

