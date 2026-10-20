# VL53L0X Write-Read Pair Tables

## Full Table

| File | Function | Write Reg | Read Reg | Write Line | Read Line |
|---|---|---:|---:|---:|---:|
| vl53l0x_api_calibration.c | VL53L0X_ref_calibration_io | 0xFF | 0xCB | 1110 | 1114 |
| vl53l0x_api_calibration.c | VL53L0X_ref_calibration_io | 0xFF | 0xEE | 1110 | 1116 |
| vl53l0x_api_core.c | VL53L0X_device_read_strobe | 0x83 | 0x83 | 187 | 195 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0xFF | 0x83 | 253 | 254 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 265 | 267 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 272 | 274 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 285 | 287 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 298 | 300 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 302 | 304 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 306 | 308 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 317 | 319 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 330 | 334 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 344 | 348 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 363 | 365 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 367 | 369 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 371 | 373 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 378 | 380 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 385 | 387 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0x94 | 0x90 | 392 | 394 |
| vl53l0x_api_core.c | VL53L0X_get_info_from_device | 0xFF | 0x83 | 401 | 402 |
| vl53l0x_api.c | VL53L0X_DataInit | 0x00 | 0x91 | 483 | 484 |
| vl53l0x_api.c | VL53L0X_GetSpadAmbientDamperFactor | 0xFF | 0x42 | 3090 | 3091 |
| vl53l0x_api.c | VL53L0X_GetSpadAmbientDamperThreshold | 0xFF | 0x40 | 3056 | 3057 |
| vl53l0x_api.c | VL53L0X_GetStopCompletedStatus | 0xFF | 0x04 | 2941 | 2944 |
| vl53l0x_api.c | VL53L0X_StaticInit | 0xFF | 0x84 | 666 | 667 |

## Deduplicated Pair Counts

| Write Reg | Read Reg | Occurrences |
|---:|---:|---:|
| 0x94 | 0x90 | 15 |
| 0xFF | 0x83 | 2 |
| 0x00 | 0x91 | 1 |
| 0x83 | 0x83 | 1 |
| 0xFF | 0x04 | 1 |
| 0xFF | 0x40 | 1 |
| 0xFF | 0x42 | 1 |
| 0xFF | 0x84 | 1 |
| 0xFF | 0xCB | 1 |
| 0xFF | 0xEE | 1 |
