# DHT Sensor Error Handling Improvements

## Problem
Occasional checksum errors when reading DHT sensor data:
```
E (95616) dht_reader: Checksum failed: 82 != 95
W (95616) dht_reader: DHT read failed: ESP_ERR_INVALID_CRC
```

## Root Causes

1. **WiFi Interference**: WiFi operations cause interrupts that disrupt the precise microsecond-level timing required by DHT sensors
2. **Timing Sensitivity**: DHT22/AM2301 sensors require very precise timing (±1μs) for reliable communication
3. **Signal Integrity**: Electrical noise, long wires, or poor connections can corrupt data bits
4. **Sensor Characteristics**: DHT sensors are known to occasionally produce bad readings even in ideal conditions

## Solutions Implemented

### 1. Automatic Retry Mechanism
- **3 retry attempts** before giving up on a reading
- 500ms delay between retries to allow sensor recovery
- Reduces log spam by only logging warnings on first attempt
- Tracks consecutive failures to detect sensor disconnection

### 2. Interrupt Protection
- **Disables interrupts** during critical timing section (data reading)
- Prevents WiFi and other system interrupts from disrupting DHT timing
- Re-enables interrupts immediately after data collection
- Properly handles interrupt state on all error paths

### 3. Improved Timing
- Added 30μs delay after start signal before switching to input mode
- Gives DHT sensor more stable transition time

### 4. Better Error Logging
- Changed checksum error from `ESP_LOGE` to `ESP_LOGD` (debug level)
- Reduces console spam for occasional expected failures
- Added detailed data dump in debug logs for troubleshooting
- Warns if 5+ consecutive failures occur (likely hardware issue)

## Expected Behavior After Fix

### Normal Operation
- Occasional checksum errors will be **automatically retried**
- Most errors will be resolved on retry without user-visible logs
- Only persistent failures (after 3 attempts) will show warnings

### Hardware Issues
- If sensor is disconnected or faulty: warning after 5 consecutive failures
- Helps distinguish between occasional errors (normal) and hardware problems

## Testing Recommendations

1. **Normal Operation**: Should see successful readings every 2 seconds
2. **WiFi Activity**: Errors should be automatically corrected by retry mechanism
3. **Hardware Check**: If you see "5 consecutive failures" warning, check:
   - DHT sensor connections
   - Power supply (3.3V or 5V depending on sensor)
   - Pull-up resistor (4.7kΩ - 10kΩ recommended)
   - Wire length (keep under 20cm for best results)

## Additional Improvements (Optional)

If you still experience issues, consider:

1. **Hardware Pull-up**: Add external 4.7kΩ resistor between DATA and VCC
2. **Shielded Cable**: Use shielded wire if sensor is far from ESP32
3. **Power Filtering**: Add 0.1μF capacitor near DHT sensor VCC/GND
4. **Lower WiFi Priority**: Reduce WiFi task priority if DHT is critical
5. **Increase Read Interval**: Change from 2s to 3-5s between reads

## Code Changes Summary

**File**: `components/libs/dht_reader/dht_reader.c`

- Added retry loop with configurable `MAX_RETRIES = 3`
- Added interrupt disable/enable around timing-critical sections
- Improved error logging and consecutive failure tracking
- Added 30μs stabilization delay after start signal
- Changed checksum error log level from ERROR to DEBUG
