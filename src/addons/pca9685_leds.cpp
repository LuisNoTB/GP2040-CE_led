#include "addons/pca9685_leds.h"
#include "storagemanager.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// Registros PCA9685
#define REG_MODE1     0x00
#define REG_MODE2     0x01
#define REG_PRESCALE  0xFE
#define REG_LED0_ON_L 0x06

// Canal del PCA9685 (0..15) para cada LED, en el mismo orden que la tabla
// de botones de abajo. Cambia esto según como cableaste cada LED.
static const uint8_t LED_CHANNEL[PCA9685_NUM_LEDS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

// Qué botón del gamepad enciende cada LED. Ajusta a tu hitbox.
// El orden debe coincidir con LED_CHANNEL.
static bool isPressed(Gamepad *gp, int led) {
    switch (led) {
        case 0:  return gp->pressedLeft();
        case 1:  return gp->pressedDown();
        case 2:  return gp->pressedRight();
        case 3:  return gp->pressedUp();
        case 4:  return gp->pressedB3();
        case 5:  return gp->pressedB4();
        case 6:  return gp->pressedR1();
        case 7:  return gp->pressedL1();
        case 8:  return gp->pressedB1();
        case 9:  return gp->pressedB2();
        case 10: return gp->pressedR2();
        case 11: return gp->pressedL2();
        case 12: return gp->pressedS2();  // 13er botón: cámbialo si es otro
        default: return false;
    }
}

bool PCA9685Leds::available() {
    return true; // siempre activo (sin config en la web)
}

bool PCA9685Leds::writeRegs(uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[1 + PCA9685_NUM_LEDS * 4];
    if (len > sizeof(buf) - 1) return false;
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) buf[1 + i] = data[i];
    int r = i2c_write_timeout_us(PCA9685_I2C_BLOCK, PCA9685_I2C_ADDR,
                                 buf, len + 1, false, 5000);
    return r == (int)(len + 1);
}

void PCA9685Leds::setup() {
    i2c_init(PCA9685_I2C_BLOCK, PCA9685_I2C_SPEED);
    gpio_set_function(PCA9685_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PCA9685_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PCA9685_SDA_PIN);
    gpio_pull_up(PCA9685_SCL_PIN);

    uint8_t v;
    v = 0x10; // MODE1: sleep
    if (!writeRegs(REG_MODE1, &v, 1)) { ready = false; return; }
    v = 0x04; // MODE2: OUTDRV totem-pole, no invertido
    writeRegs(REG_MODE2, &v, 1);
    v = 121;  // prescale ~50 Hz (irrelevante para on/off, ok para PWM)
    writeRegs(REG_PRESCALE, &v, 1);
    v = 0x20; // MODE1: despierta, auto-incremento
    writeRegs(REG_MODE1, &v, 1);
    sleep_us(600);
    v = 0xA0; // MODE1: restart + auto-incremento
    writeRegs(REG_MODE1, &v, 1);

    ready = true;
    lastMask = 0xFFFF;
    pushState(0); // todo apagado al inicio
}

void PCA9685Leds::pushState(uint16_t mask) {
    uint8_t data[16 * 4] = {0};
    // Por defecto: canales apagados (bit "full off" en OFF_H)
    for (int ch = 0; ch < 16; ch++) data[ch * 4 + 3] = 0x10;

    for (int i = 0; i < PCA9685_NUM_LEDS; i++) {
        uint8_t ch = LED_CHANNEL[i] & 0x0F;
        uint8_t *p = &data[ch * 4];
        if (mask & (1u << i)) {
            if (PCA9685_BRIGHTNESS >= 4095) {
                p[0] = 0; p[1] = 0x10; // full on
                p[2] = 0; p[3] = 0;
            } else {
                uint16_t off = PCA9685_BRIGHTNESS;
                p[0] = 0; p[1] = 0;
                p[2] = off & 0xFF; p[3] = (off >> 8) & 0x0F;
            }
        }
    }
    // Una sola transacción para los 16 canales (64 bytes)
    uint8_t buf[1 + 64];
    buf[0] = REG_LED0_ON_L;
    for (int i = 0; i < 64; i++) buf[1 + i] = data[i];
    i2c_write_timeout_us(PCA9685_I2C_BLOCK, PCA9685_I2C_ADDR, buf, 65, false, 5000);
}

void PCA9685Leds::process() {
    if (!ready) return;

    Gamepad *gp = Storage::getInstance().GetGamepad();
    uint16_t mask = 0;
    for (int i = 0; i < PCA9685_NUM_LEDS; i++) {
        if (isPressed(gp, i)) mask |= (1u << i);
    }
    if (mask != lastMask) {   // solo escribe si cambió algo
        pushState(mask);
        lastMask = mask;
    }
}
