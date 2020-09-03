/**
 * @file yam_frame_tool.h
 */

#ifndef __YAM_FRAME_TOOL_H
#define __YAM_FRAME_TOOL_H

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include <stdint.h>

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate modbus CRC from a frame buffer.
 * @param buf the frame buffer
 * @param n lenght of the buffer
 * @return the resulted crc16 value
 */
uint16_t modbus_crc(const char *buf, size_t n);

#ifdef __cplusplus
} /* extern "C" */
#endif

/**********************
 *      MACROS
 **********************/

#endif /* __YAM_FRAME_TOOL_H */
