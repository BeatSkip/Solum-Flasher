#ifndef ZBS_INTERFACE_H
#define ZBS_INTERFACE_H

#include <stdio.h>
#include <stdint.h>
#include <SPI.h>

// ZBS Interface Library for RP2040 (Bitbanged SPI)
// Author: Oscar Spierings
// Based on original work by Aaron Christophel (ATCnetz.de)

#include <Arduino.h>

class ZBS_interface
{
public:
     uint8_t begin(uint8_t SS, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t RESET, uint8_t POWER);
    void set_power(uint8_t state);
    void enable_debug();
    void reset();
    void write_byte(uint8_t cmd, uint8_t addr, uint8_t data);
    uint8_t read_byte(uint8_t cmd, uint8_t addr);
    void write_flash(uint16_t addr, uint8_t data);
    uint8_t read_flash(uint16_t addr);
    void write_ram(uint8_t addr, uint8_t data);
    uint8_t read_ram(uint8_t addr);
    void write_sfr(uint8_t addr, uint8_t data);
    uint8_t read_sfr(uint8_t addr);
    uint8_t check_connection();
    uint8_t select_flash(uint8_t page);
    void erase_flash();
    void erase_infoblock();

private:
    
	int8_t _SS_PIN, _CLK_PIN, _MOSI_PIN, _MISO_PIN, _RESET_PIN, _POWER_PIN;
    void send_byte(uint8_t data);
    uint8_t read_byte();
    void delayMicroseconds(unsigned int us);
    uint32_t after_byte_delay = 10;

    typedef enum
    {
        ZBS_CMD_W_RAM = 0x02,
        ZBS_CMD_R_RAM = 0x03,
        ZBS_CMD_W_FLASH = 0x08,
        ZBS_CMD_R_FLASH = 0x09,
        ZBS_CMD_W_SFR = 0x12,
        ZBS_CMD_R_SFR = 0x13,
        ZBS_CMD_ERASE_FLASH = 0x88,
        ZBS_CMD_ERASE_INFOBLOCK = 0x48,
    } ZBS_CMD_LIST;

    typedef enum
    {
        ZBS_ON = 1,
        ZBS_OFF = 0,
    } ZBS_POWER_STATE;
};

extern ZBS_interface zbs;

#endif
