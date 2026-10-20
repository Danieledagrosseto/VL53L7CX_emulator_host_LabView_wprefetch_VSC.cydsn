#include "vl53l7cx_to_LabView.h"
#include <stddef.h>

#if defined(VL53L7CX_DISABLE_DISTANCE_MM) || \
    defined(VL53L7CX_DISABLE_TARGET_STATUS) || \
    defined(VL53L7CX_DISABLE_NB_TARGET_DETECTED)
#error "Enable distance, target status and target count in ST platform.h"
#endif
#ifdef VL53L7CX_USE_RAW_FORMAT
#error "UART protocol v1 requires normal ULD units; disable USE_RAW_FORMAT"
#endif

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint16_t crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFu;
    uint16_t i;
    uint8_t bit;
    for (i = 0u; i < length; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (bit = 0u; bit < 8u; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)(((uint32_t)crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)((uint32_t)crc << 1);
            }
        }
    }
    return crc;
}

static void header(uint8_t *packet, uint16_t sequence, uint32_t timestamp_ms)
{
    packet[0] = 0xA5u;
    packet[1] = 0x5Au;
    packet[2] = 0xC3u;
    packet[3] = 0x3Cu;
    packet[4] = 1u;
    packet[5] = 1u;
    put_u16(&packet[6], sequence);
    put_u32(&packet[8], timestamp_ms);
    packet[12] = VL53L7CX_LV_ZONE_COUNT;
    packet[13] = 1u;
    put_u16(&packet[14], VL53L7CX_LV_PAYLOAD_SIZE);
}

static void finish(uint8_t *packet)
{
    put_u16(&packet[272], crc16(&packet[4], 268u));
}

static void transmit(VL53L7CX_LV_Context *context, const uint8_t *packet)
{
    uint16_t i;
    for (i = 0u; i < VL53L7CX_LV_PACKET_SIZE; ++i) {
        context->write_byte(packet[i]);
    }
    context->next_sequence = (uint16_t)(context->next_sequence + 1u);
}

uint8_t VL53L7CX_LV_Init(VL53L7CX_LV_Context *context,
                         VL53L7CX_LV_WriteByte write_byte)
{
    if (context == NULL || write_byte == NULL) {
        return VL53L7CX_LV_INVALID_ARGUMENT;
    }
    context->write_byte = write_byte;
    context->next_sequence = 0u;
    return VL53L7CX_LV_OK;
}

uint8_t VL53L7CX_LV_BuildPacket(
    uint8_t *packet, uint16_t capacity, uint16_t sequence,
    uint32_t timestamp_ms, const int16_t *distance_mm,
    const uint8_t *target_status, const uint8_t *nb_target_detected)
{
    uint16_t zone;
    if (packet == NULL || capacity < VL53L7CX_LV_PACKET_SIZE ||
        distance_mm == NULL || target_status == NULL ||
        nb_target_detected == NULL) {
        return VL53L7CX_LV_INVALID_ARGUMENT;
    }
    header(packet, sequence, timestamp_ms);
    for (zone = 0u; zone < VL53L7CX_LV_ZONE_COUNT; ++zone) {
        put_u16(&packet[16u + 2u * zone], (uint16_t)distance_mm[zone]);
        packet[144u + zone] = target_status[zone];
        packet[208u + zone] = nb_target_detected[zone];
    }
    finish(packet);
    return VL53L7CX_LV_OK;
}

uint8_t VL53L7CX_LV_SendArrays(
    VL53L7CX_LV_Context *context, uint32_t timestamp_ms,
    const int16_t *distance_mm, const uint8_t *target_status,
    const uint8_t *nb_target_detected)
{
    uint8_t packet[VL53L7CX_LV_PACKET_SIZE];
    uint8_t result;
    if (context == NULL || context->write_byte == NULL) {
        return VL53L7CX_LV_INVALID_ARGUMENT;
    }
    result = VL53L7CX_LV_BuildPacket(packet, sizeof packet,
        context->next_sequence, timestamp_ms, distance_mm,
        target_status, nb_target_detected);
    if (result != VL53L7CX_LV_OK) {
        return result;
    }
    transmit(context, packet);
    return VL53L7CX_LV_OK;
}

uint8_t VL53L7CX_LV_SendResults(
    VL53L7CX_LV_Context *context,
    const VL53L7CX_ResultsData *results, uint32_t timestamp_ms)
{
    uint8_t packet[VL53L7CX_LV_PACKET_SIZE];
    uint16_t zone;
    uint16_t index;
    if (context == NULL || context->write_byte == NULL || results == NULL) {
        return VL53L7CX_LV_INVALID_ARGUMENT;
    }
    header(packet, context->next_sequence, timestamp_ms);
    for (zone = 0u; zone < VL53L7CX_LV_ZONE_COUNT; ++zone) {
        index = (uint16_t)(zone * VL53L7CX_NB_TARGET_PER_ZONE);
        put_u16(&packet[16u + 2u * zone],
                (uint16_t)results->distance_mm[index]);
        packet[144u + zone] = results->target_status[index];
        packet[208u + zone] = results->nb_target_detected[zone];
    }
    finish(packet);
    transmit(context, packet);
    return VL53L7CX_LV_OK;
}
