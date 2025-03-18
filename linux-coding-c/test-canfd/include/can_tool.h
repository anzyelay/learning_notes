#ifndef __CAN_TOOL_H__
#define __CAN_TOOL_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief   Get the Value From Canfd Msg object
 *
 * @param   data       CANFD data payload
 * @param   startByte  CANFD startByte
 * @param   startBit   CANFD startBit
 * @param   bitLength  bitLength
 * @return  ret
 */
uint64_t get_intel_data_from(const uint8_t *data, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength);

/**
 * @brief Insert a Value to Can Msg object
 *
 * @param   data       CANFD data payload
 * @param   val        value of signal data
 * @param   startByte  CANFD startByte
 * @param   startBit   CANFD startBit
 * @param   bitLength  bitLength
 * @return  void
 */
void set_intel_data_from(uint8_t *data, uint64_t val, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength);

uint64_t get_motorola_backward_data_from(const uint8_t *data, uint8_t dlc, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength);

void set_motorola_backward_data_from(uint8_t *data, uint64_t val, uint8_t dlc ,const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength);

#endif
