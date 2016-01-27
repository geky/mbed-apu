
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
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1},
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
Channel::Channel() {
    _period = 0;
    _volume = 15;
}

// Enable/disable specified channel
void Channel::enable() {
    _period = 0xfff;
}

void Channel::disable() {
    _period = 0;
}

// Emulate a channel
void Channel::update(int cycles) {
    if (!_period || !_volume) return;
    _timer -= cycles;

    while (_timer <= 0) {
        _timer += _period;
        tick();
    }
}

// Set the note being played by a channel
// Note value starts at A0
void Channel::note(uint8_t note) {
    _period = PTABLE[note - 9];
    _timer = _period;
}

// Set the volume of a channel
void Channel::volume(uint8_t volume) {
    _volume = volume;
}

// Set the duty cycle of a channel
void Channel::duty(uint8_t duty) {
    _duty = duty;
}



// Square channel
void Square::tick() {
    _tick = (_tick+1) & 0x7;
}

uint8_t Square::output() {
    return _volume * SQUARE[_duty][_tick];
}

// Triangel channel
void Triangle::update(int cycles) {
    Channel::update(2*cycles);
}

void Triangle::tick() {
    _tick = (_tick+1) & 0x1f;
}

uint8_t Triangle::output() {
    return TRIANGLE[_tick];
}

// Noise channel
void Noise::note(uint8_t note) {
    _period = NTABLE[note & 0xf];
    _timer = _period;
}

void Noise::tick() {
    _tick = 1 & (_shift ^ (_shift >> (_duty ? 6 : 1)));
    _shift = (_shift >> 1) | (_tick << 14);
}

uint8_t Noise::output() {
    return (_volume >> 1) * _tick;
}

