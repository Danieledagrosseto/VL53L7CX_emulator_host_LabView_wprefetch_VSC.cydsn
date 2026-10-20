/* VL53L7CX UART protocol v1: normal ULD millimetre output, 8x8, target 0.
 * C99; no allocation and no dependency on a particular UART component name.
 * Add the ST ULD include directory to your project.
 */
#ifndef VL53L7CX_TO_LABVIEW_H
#define VL53L7CX_TO_LABVIEW_H

#include <stdint.h>
#include "vl53l7cx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L7CX_LV_ZONE_COUNT 64u
#define VL53L7CX_LV_PAYLOAD_SIZE 256u
#define VL53L7CX_LV_PACKET_SIZE 274u

/* The callback must transmit or copy the byte before returning, in order.
 * Example: static void tx(uint8_t b) { UART_1_PutChar(b); }
 * The UART must already be started. Use a blocking callback / sufficiently
 * buffered UART. The module cannot detect UART failures with this interface.
 */
typedef void (*VL53L7CX_LV_WriteByte)(uint8_t byte);

typedef struct {
    VL53L7CX_LV_WriteByte write_byte;
    uint16_t next_sequence;
} VL53L7CX_LV_Context;

enum {
    VL53L7CX_LV_OK = 0,
    VL53L7CX_LV_INVALID_ARGUMENT = 1
};

/* Packet offsets (zero based), little endian for all multibyte fields:
 *   0..3   A5 5A C3 3C
 *   4      version = 1
 *   5      type = 1
 *   6..7   sequence U16 (wraps modulo 65536)
 *   8..11  timestamp_ms U32 (acquisition time, not UART transmission time)
 *  12      zone count = 64
 *  13      transmitted targets per zone = 1 (always target index 0)
 *  14..15  payload length = 256
 *  16..143 distance[0..63], signed I16 mm
 * 144..207 target_status[0..63], U8
 * 208..271 nb_target_detected[0..63], U8
 * 272..273 CRC-16/CCITT-FALSE over bytes 4..271, little endian.
 * CRC: poly 0x1021, init 0xFFFF, refin=false, refout=false, xorout=0.
 * Arrays remain in ST zone order. No filtering, averaging, flips or rounding.
 */
uint8_t VL53L7CX_LV_Init(VL53L7CX_LV_Context *context,
                         VL53L7CX_LV_WriteByte write_byte);

/* Builds a packet without sending it. Buffer capacity must be >= 274 bytes.
 * The three input arrays must each contain 64 elements, not overlap packet,
 * and use normal ULD units. Caller supplies sequence and timestamp.
 */
uint8_t VL53L7CX_LV_BuildPacket(
    uint8_t *packet, uint16_t capacity, uint16_t sequence,
    uint32_t timestamp_ms, const int16_t *distance_mm,
    const uint8_t *target_status, const uint8_t *nb_target_detected);

/* Sends stored, already selected target-0 arrays. Uses a 274-byte local buffer.
 * Incrementing sequence records submitted packets, not delivery confirmation.
 */
uint8_t VL53L7CX_LV_SendArrays(
    VL53L7CX_LV_Context *context, uint32_t timestamp_ms,
    const int16_t *distance_mm, const uint8_t *target_status,
    const uint8_t *nb_target_detected);

/* Sends target 0 of a successfully acquired 8x8 Results structure.
 * Respects VL53L7CX_NB_TARGET_PER_ZONE stride. Caller must guarantee 8x8
 * acquisition: Results does not carry a resolution field.
 * Results must not be modified concurrently while this function builds it.
 * Uses a 274-byte local buffer; do not call concurrently on the same context.
 */
uint8_t VL53L7CX_LV_SendResults(
    VL53L7CX_LV_Context *context,
    const VL53L7CX_ResultsData *results, uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif
#endif
