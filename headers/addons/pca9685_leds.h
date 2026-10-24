#ifndef _PCA9685_LEDS_H_
#define _PCA9685_LEDS_H_

#include "gpaddon.h"
#include <string>
#include <stdint.h>

// ---- Configuración (edita aquí) -------------------------------------------
#define PCA9685_I2C_BLOCK   i2c0      // GP0 = SDA, GP1 = SCL -> i2c0
#define PCA9685_SDA_PIN     0
#define PCA9685_SCL_PIN     1
#define PCA9685_I2C_ADDR    0x40
#define PCA9685_I2C_SPEED   400000    // 400 kHz
#define PCA9685_BRIGHTNESS  4095      // 0..4095 (4095 = 100%)
#define PCA9685_NUM_LEDS    13
// ---------------------------------------------------------------------------

#define PCA9685LedsName "PCA9685Leds"

class PCA9685Leds : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void preprocess() {}
    virtual void process();
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return PCA9685LedsName; }

private:
    bool writeRegs(uint8_t reg, const uint8_t *data, size_t len);
    void pushState(uint16_t mask);

    bool     ready = false;
    uint16_t lastMask = 0xFFFF; // fuerza primera escritura
};

#endif
