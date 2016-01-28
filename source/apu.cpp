
// NES APU
//

#include "apu/apu.h"

using namespace apu;


// APU lifetime
APU::APU(Channel **channels, unsigned count, PinName pin)
        : _channels(channels), _count(count), _dac(pin) {
}


// APU Emulation
void APU::tick() {
    unsigned output = 0;

    for (unsigned i = 0; i < _count; i++) {
        _channels[i]->update(APU_SCALE);
        output += _channels[i]->output();
    }

    if (output > 0x03f) output = 0x03f;
    _output = output;
    _dac.write_u16(output << 10);
}

void APU::start() {
    timestamp_t delay = 1000000 / (APU_FREQ/APU_SCALE);
    _ticker.attach_us(this, &APU::tick, delay);
}

void APU::stop() {
    _ticker.detach();
}


// Enable/disable specified channel
void APU::enable(unsigned channel) {
    if (channel >= _count) return;
    _channels[channel]->enable();
}

void APU::disable(unsigned channel) {
    if (channel >= _count) return;
    _channels[channel]->disable();
}

// Set the note being played by a channel
// Note value starts at A0
void APU::note(unsigned channel, uint8_t note) {
    if (channel >= _count) return;
    _channels[channel]->note(note);
}

// Set the volume of a channel
void APU::volume(unsigned channel, uint8_t volume) {
    if (channel >= _count) return;
    _channels[channel]->volume(volume);
}

// Set the duty cycle of a channel
void APU::duty(unsigned channel, uint8_t duty) {
    if (channel >= _count) return;
    _channels[channel]->duty(duty);
}

// Get the current amplitude of the APU
uint8_t APU::output() {
    return _output;
}

