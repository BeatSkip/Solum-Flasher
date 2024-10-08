#include <Arduino.h>
#include "main.h"
#include "zbs_interface.h"

// Pin definitions
#define EslMosi 11
#define EslMiso 12
#define EslSck  10
#define EslCs   13

#define EslReset 15
#define EslTest  14
#define EslPower  7
#define dbgRx     9
#define dbgTx     8

#define ledPin 16

uint32_t FLASHER_VERSION = 0x00000020;

StatusLed led(1, ledPin);

void setup() {
  Serial.begin(115200);
  led.set(BLACK);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  led.begin();
  pinMode(dbgRx, INPUT);
  

  while (Serial.available()) {
    Serial.read(); // Flushing UART
  }
  led.set(GREEN);
}

void loop() {
  UART_handler();
}

int UART_rx_state = 0;
uint8_t expected_len = 0;
uint32_t last_rx_time = 0;
uint8_t UART_rx_buffer[0x200] = {0};
int curr_data_pos = 0;
uint32_t CRC_calc = 0xAB34;
uint32_t CRC_in = 0;
uint8_t UART_CMD = 0;

void UART_handler() {
  while (Serial.available()) {
    last_rx_time = millis();
    uint8_t curr_char = Serial.read();
    switch (UART_rx_state) {
      case 0:
        if (curr_char == 'A') UART_rx_state++;
        break;
      case 1:
        if (curr_char == 'T') UART_rx_state++;
        else UART_rx_state = 0;
        break;
      case 2:
        UART_CMD = curr_char;
        CRC_calc = 0xAB34;
        CRC_calc += curr_char;
        UART_rx_state++;
        break;
      case 3:
        expected_len = curr_char;
        CRC_calc += curr_char;
        curr_data_pos = 0;
        UART_rx_state = (expected_len == 0) ? 5 : 4;
        break;
      case 4:
        CRC_calc += curr_char;
        UART_rx_buffer[curr_data_pos++] = curr_char;
        if (curr_data_pos == expected_len) UART_rx_state++;
        break;
      case 5:
        CRC_in = curr_char << 8;
        UART_rx_state++;
        break;
      case 6:
        if ((CRC_calc & 0xffff) == (CRC_in | curr_char)) {
          led.setSilent(CYAN);
          led.Toggle();
          handle_uart_cmd(UART_CMD, UART_rx_buffer, expected_len);
        }
        UART_rx_state = 0;
        break;
    }
  }
  if (UART_rx_state && (millis() - last_rx_time >= 10)) {
    UART_rx_state = 0;
  }
}

typedef enum {
  CMD_GET_VERSION = 1,
  CMD_RESET_MCU = 2,
  CMD_ZBS_BEGIN = 10,
  CMD_RESET_ZBS = 11,
  CMD_SELECT_PAGE = 12,
  CMD_SET_POWER = 13,
  CMD_READ_RAM = 20,
  CMD_WRITE_RAM = 21,
  CMD_READ_FLASH = 22,
  CMD_WRITE_FLASH = 23,
  CMD_READ_SFR = 24,
  CMD_WRITE_SFR = 25,
  CMD_ERASE_FLASH = 26,
  CMD_ERASE_INFOBLOCK = 27,
  CMD_SAVE_MAC_FROM_FW = 40,
  CMD_PASS_THROUGH = 50,
} ZBS_UART_PROTO;

uint8_t temp_buff[0x200] = {0};

void handle_uart_cmd(uint8_t cmd, uint8_t *cmd_buff, uint8_t len) {
  switch (cmd) {
    case CMD_GET_VERSION:
      led.set(BLUE);
      temp_buff[0] = FLASHER_VERSION >> 24;
      temp_buff[1] = FLASHER_VERSION >> 16;
      temp_buff[2] = FLASHER_VERSION >> 8;
      temp_buff[3] = FLASHER_VERSION;
      send_uart_answer(cmd, temp_buff, 4);
      break;
    case CMD_RESET_MCU:
      send_uart_answer(cmd, NULL, 0);
      delay(100);
      rp2040.reboot();
      break;
    case CMD_ZBS_BEGIN:
      temp_buff[0] = zbs.begin(EslCs, EslSck, EslMosi, EslMiso, EslReset, EslPower);
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_RESET_ZBS:
      zbs.reset();
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_SELECT_PAGE:
      temp_buff[0] = zbs.select_flash(cmd_buff[0] ? 1 : 0);
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_SET_POWER:
      zbs.set_power(cmd_buff[0] ? 1 : 0);
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_READ_RAM:
      temp_buff[0] = zbs.read_ram(cmd_buff[0]);
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_WRITE_RAM:
      zbs.write_ram(cmd_buff[0], cmd_buff[1]);
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_READ_FLASH:
      for (int i = 0; i < cmd_buff[0]; i++) {
        temp_buff[i] = zbs.read_flash((cmd_buff[1] << 8 | cmd_buff[2]) + i);
      }
      send_uart_answer(cmd, temp_buff, cmd_buff[0]);
      break;
    case CMD_WRITE_FLASH:
      if (cmd_buff[0] >= (0xff - 3)) {
        temp_buff[0] = 0xEE;
        send_uart_answer(cmd, temp_buff, 1);
        break;
      }
      for (int i = 0; i < cmd_buff[0]; i++) {
        if (cmd_buff[3 + i] != 0xff) {
          zbs.write_flash((cmd_buff[1] << 8 | cmd_buff[2]) + i, cmd_buff[3 + i]);
          for (int retry = 0; retry < 4; retry++) {
            if (zbs.read_flash((cmd_buff[1] << 8 | cmd_buff[2]) + i) == cmd_buff[3 + i]) {
              break;
            }
            zbs.write_flash((cmd_buff[1] << 8 | cmd_buff[2]) + i, cmd_buff[3 + i]);
            if (retry == 3) {
              temp_buff[0] = 0;
              send_uart_answer(cmd, temp_buff, 1);
              return;
            }
          }
        }
      }
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_READ_SFR:
      temp_buff[0] = zbs.read_sfr(cmd_buff[0]);
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_WRITE_SFR:
      zbs.write_sfr(cmd_buff[0], cmd_buff[1]);
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_ERASE_FLASH:
      zbs.erase_flash();
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_ERASE_INFOBLOCK:
      zbs.erase_infoblock();
      temp_buff[0] = 1;
      send_uart_answer(cmd, temp_buff, 1);
      break;
    case CMD_SAVE_MAC_FROM_FW:
      handle_save_mac_from_fw();
      break;
    case CMD_PASS_THROUGH:
      temp_buff[0] = 0; // Not supported on RP2040
      send_uart_answer(cmd, temp_buff, 1);
      break;
  }
}

void handle_save_mac_from_fw() {
  uint8_t temp[8];
  zbs.select_flash(0);
  
  // Get original MAC from stock firmware
  for (uint8_t c = 0; c < 6; c++) {
    temp_buff[c + 2] = zbs.read_flash(0xFC06 + c);
  }
  
  // Try to recognize device type
  for (uint8_t c = 0; c < 8; c++) {
    temp[c] = zbs.read_flash(0x08 + c);
  }
  
  const uint8_t val29[8] = {0x7d, 0x22, 0xff, 0x02, 0xa4, 0x58, 0xf0, 0x90};
  const uint8_t val29_1[8] = {0x7d, 0x22, 0xff, 0x02, 0xaa, 0xb3, 0xf0, 0x90};
  const uint8_t val154[8] = {0xa1, 0x23, 0x22, 0x02, 0xa4, 0xc3, 0xe4, 0xf0};
  const uint8_t val42[8] = {0xDF, 0x22, 0x22, 0x02, 0xAD, 0x35, 0xAE, 0x04};
  
  bool validMac = true;
  if (memcmp(temp, val29, 8) == 0 || memcmp(temp, val29_1, 8) == 0) {
    temp_buff[6] = 0x3B;
    temp_buff[7] = 0x10;
  } else if (memcmp(temp, val154, 8) == 0) {
    temp_buff[6] = 0x34;
    temp_buff[7] = 0x10;
  } else if (memcmp(temp, val42, 8) == 0) {
    temp_buff[6] = 0x48;
    temp_buff[7] = 0x30;
  } else {
    validMac = false;
    temp_buff[6] = 0xFF;
    temp_buff[7] = 0xFF;
  }
  
  // Calculate checksum
  uint8_t xorchk = 0;
  for (uint8_t c = 2; c < 8; c++) {
    xorchk ^= (temp_buff[c] & 0x0F);
    xorchk ^= (temp_buff[c] >> 4);
  }
  temp_buff[7] |= xorchk;
  
  // Check if there's already a MAC in the infoblock
  zbs.select_flash(1);
  bool infoblockEmpty = true;
  for (uint8_t c = 0; c < 8; c++) {
    if (zbs.read_flash(0x10 + c) != 0xFF) {
      infoblockEmpty = false;
      break;
    }
  }
  
  if (infoblockEmpty && validMac) {
    for (uint8_t c = 0; c < 8; c++) {
      zbs.write_flash(0x17 - c, temp_buff[c]);
    }
    temp_buff[0] = 1;
  } else if (!infoblockEmpty) {
    bool macAlreadyMatches = true;
    for (uint8_t c = 0; c < 8; c++) {
      if (temp_buff[c] != zbs.read_flash(0x17 - c)) {
        macAlreadyMatches = false;
        break;
      }
    }
    temp_buff[0] = macAlreadyMatches ? 1 : 0;
  } else {
    temp_buff[0] = 0;
  }
  
  send_uart_answer(CMD_SAVE_MAC_FROM_FW, temp_buff, 1);
}

uint8_t answer_buffer[0x200] = {0};

void send_uart_answer(uint8_t answer_cmd, uint8_t *ans_buff, uint8_t len) {
  uint32_t CRC_value = 0xAB34;
  answer_buffer[0] = 'A';
  answer_buffer[1] = 'T';
  answer_buffer[2] = answer_cmd;
  CRC_value += answer_cmd;
  answer_buffer[3] = len;
  CRC_value += len;
  for (int i = 0; i < len; i++) {
    answer_buffer[4 + i] = ans_buff[i];
    CRC_value += ans_buff[i];
  }
  answer_buffer[2 + 2 + len] = CRC_value >> 8;
  answer_buffer[2 + 2 + len + 1] = CRC_value;
  Serial.write(answer_buffer, 2 + 2 + len + 2);
}