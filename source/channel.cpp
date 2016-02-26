
// NES APU Channels
//

#include "apu/apu.h"

using namespace apu;



// Lookup tables for different waveforms
const static unsigned char TRIANGLE[32] = {
    15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
};

const static unsigned char SQUARE[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};


// Period lookup table
const static unsigned short PTABLE[] = {
    0x7f1, 0x77f, 0x713, 0x6ad, 0x64d, 0x5f3, 0x59d, 0x54c,
    0x500, 0x4b8, 0x474, 0x434, 0x3f8, 0x3bf, 0x389, 0x356,
    0x326, 0x2f9, 0x2ce, 0x2a6, 0x280, 0x25c, 0x23a, 0x21a,
    0x1fb, 0x1df, 0x1c4, 0x1ab, 0x193, 0x17c, 0x167, 0x152,
    0x13f, 0x12d, 0x11c, 0x10c, 0xfd,  0xef,  0xe1,  0xd5,
    0xc9,  0xbd,  0xb3,  0xa9,  0x9f,  0x96,  0x8e,  0x86,
    0x7e,  0x77,  0x70,  0x6a,  0x64,  0x5e,  0x59,  0x54,
    0x4f,  0x4b,  0x46,  0x42,  0x3f,  0x3b,  0x38,  0x34,
    0x31,  0x2f,  0x2c,  0x29,  0x27,  0x25,  0x23,  0x21,
    0x1f,  0x1d,  0x1b,  0x1a,  0x18,  0x17,  0x15,  0x14,
    0x13,  0x12,  0x11,  0x10,  0xf,   0xe,   0xd,   0x0
};

// Noise period lookup table
const static unsigned short NTABLE[] = {
    0xfe4, 0x7f2, 0x3f8, 0x2fa, 0x1fc, 0x17c, 0xfe, 0xca,
    0xa0,  0x80,  0x60,  0x40,  0x20,  0x10,  0x8,  0x4
};



// General channel implementation

// Channel lifetime
Channel::Channel()
  : _output(0)
  , _period(0xfff)
  , _volume(15) {
}


// Emulate a channel
void Channel::retick(unsigned us) {
    _ticker.attach_us(this, &Channel::tick, us*1000000 / APU_FREQ);
}

void Channel::tick() {
    if (_update) {
        retick(_period + _pitch);
        _update = false;
    }

    update();
    _apu->update();
}

// Enable/disable specified channel
void Channel::enable() {
    _period = 0xfff;
    retick(_period + _pitch);
}

void Channel::disable() {
    _period = 0;
    _ticker.detach();
}


// Set the note being played by a channel
// Note value starts at A0
void Channel::note(uint8_t note) {
    set_period(to_period(note));
}

// Sets the period being played by the channel directly
// Period is based on NES clock cycles
void Channel::set_period(uint16_t period) {
    _period = period;

    if (period > 0xfff || period <= 8) {
        disable();
    } else {
        retick(_period + _pitch);
    }
}

// Adjust period without timer reset
void Channel::adjust_period(uint16_t period) {
    _period = period;
    _update = true;

    if (period > 0xfff || period <= 8) {
        disable();
    }
}

// Get the current period
uint16_t Channel::get_period() {
    return _period;
}

// Set the volume of a channel
void Channel::volume(uint8_t volume) {
    _volume = volume;
}

// Set the pitch offset of a channel
void Channel::pitch(int16_t offset) {
    _pitch = offset;
    _update = true;
}

// Set the duty cycle of a channel
void Channel::duty(uint8_t duty) {
    _duty = duty;
}

// Get the output of a channel
uint8_t Channel::output() {
    return _output;
}



// Square channel
uint16_t Square::to_period(uint8_t note) {
    return PTABLE[note - 9] << 1;
}

void Square::update() {
    _output = _volume * SQUARE[_duty][_tick];
    _tick = (_tick+1) & 0x7;
}


// Triangle channel
uint16_t Triangle::to_period(uint8_t note) {
    return PTABLE[note - 9];
}

void Triangle::update() {
    _output = _volume ? TRIANGLE[_tick] : 8;
    _tick = (_tick+1) & 0x1f;
}


// Noise channel
uint16_t Noise::to_period(uint8_t note) {
    return NTABLE[note & 0xf];
}

void Noise::update() {
    _tick = 1 & (_shift ^ (_shift >> (_duty ? 6 : 1)));
    _shift = (_shift >> 1) | (_tick << 14);
    _output = (_volume/2) * _tick;
}

