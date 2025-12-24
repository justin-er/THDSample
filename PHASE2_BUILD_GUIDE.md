# Phase 2: Build and Test Guide

## Prerequisites

1. **ESP-IDF Setup**
   - ESP-IDF v5.x installed and configured
   - ESP32-S3 toolchain
   - Python 3.x with ESP-IDF requirements

2. **Hardware**
   - ESP32-S3 development board
   - DHT22 temperature/humidity sensor
   - WS2812 LED strip (or compatible)
   - USB cable for programming

## Build Instructions

### 1. Set up ESP-IDF Environment

```bash
# Navigate to ESP-IDF directory
cd ~/esp/esp-idf

# Source the environment
. ./export.sh

# Verify setup
idf.py --version
```

### 2. Configure Project

```bash
cd /Users/amirrezaeghtedari/Documents/Projects/ESPStudy/DHTSample

# Open menuconfig (optional - defaults should work)
idf.py menuconfig
```

**Key Configuration Items:**
- Serial flasher config → Flash size (ensure adequate for OTA)
- Partition Table → Custom partition table (partitions.csv)
- Component config → HTTP Server → Max URI handlers (20)

### 3. Build Project

```bash
# Clean build (recommended for first time)
idf.py fullclean
idf.py build

# Or incremental build
idf.py build
```

**Expected Output:**
```
Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p (PORT) flash
```

### 4. Flash to ESP32

```bash
# Auto-detect port and flash
idf.py flash

# Or specify port
idf.py -p /dev/ttyUSB0 flash  # Linux
idf.py -p COM3 flash           # Windows
```

### 5. Monitor Serial Output

```bash
# Flash and monitor in one command
idf.py flash monitor

# Or just monitor
idf.py monitor

# Exit monitor: Ctrl+]
```

## Testing Guide

### Phase 1: Basic Connectivity

#### Test 1.1: AP Mode
**Expected:**
- ESP32 creates WiFi network "ESP32_AP"
- Password: "password"
- IP: 192.168.0.1

**Steps:**
1. Power on ESP32
2. Check serial monitor for:
   ```
   I (xxx) wifi_app: Access Point started
   I (xxx) wifi_app: Received: START_HTTP_SERVER
   I (xxx) http_server: HTTP server started successfully
   I (xxx) dns_server: DNS server started successfully
   ```
3. On phone/laptop, scan for WiFi
4. Connect to "ESP32_AP" with password "password"
5. Verify you get IP 192.168.0.x

✅ **Pass Criteria:** Can connect to ESP32_AP and get IP address

#### Test 1.2: Captive Portal
**Expected:**
- Browser opens automatically when connecting

**Steps:**
1. Connect to ESP32_AP from phone
2. Wait 2-3 seconds
3. Browser should auto-open

✅ **Pass Criteria:** Browser opens automatically (or shows captive portal notification)

#### Test 1.3: Web Interface
**Expected:**
- Web interface loads at 192.168.0.1

**Steps:**
1. Connect to ESP32_AP
2. Open browser
3. Navigate to http://192.168.0.1
4. Verify page loads with all sections

✅ **Pass Criteria:** Web interface displays correctly

### Phase 2: Sensor Readings

#### Test 2.1: DHT22 Sensor
**Expected:**
- Temperature and humidity display on web page
- LED color changes based on humidity

**Steps:**
1. Access web interface
2. Check "DHT22 Sensor Readings" section
3. Verify temperature and humidity values
4. Observe LED color:
   - Green: < 50% humidity
   - Orange: 50-55% humidity
   - Red: > 55% humidity

✅ **Pass Criteria:** Sensor readings update every 5 seconds, LED responds to humidity

#### Test 2.2: System Status
**Expected:**
- Heap, uptime, firmware version display

**Steps:**
1. Open browser console (F12)
2. Fetch: `http://192.168.0.1/systemStatus.json`
3. Verify JSON response:
   ```json
   {
     "heap_free": 123456,
     "heap_min": 98765,
     "uptime_seconds": 120,
     "firmware_version": "1.0.0",
     "compile_date": "Dec 24 2025",
     "compile_time": "15:30:00"
   }
   ```

✅ **Pass Criteria:** System status returns valid data

### Phase 3: WiFi Management

#### Test 3.1: WiFi Scanner
**Expected:**
- Can scan for available networks
- Networks sorted by signal strength

**Steps:**
1. Access web interface
2. Click "Scan Networks" (if UI implemented)
3. Or fetch: `http://192.168.0.1/wifiScan.json`
4. Verify list of networks with RSSI and auth type

✅ **Pass Criteria:** Scan returns available networks sorted by RSSI

#### Test 3.2: WiFi Connection
**Expected:**
- Can connect to external WiFi
- Gets IP address
- SNTP syncs time

**Steps:**
1. Access web interface
2. Enter your home WiFi SSID and password
3. Click "Connect"
4. Monitor serial output:
   ```
   I (xxx) wifi_app: Connecting to AP: YourSSID
   I (xxx) wifi_app: Connected to AP
   I (xxx) wifi_app: Got IP address: 192.168.1.150
   I (xxx) sntp_client: Time synchronized
   ```
5. Check connection info section on web page
6. Verify IP, gateway, netmask display

✅ **Pass Criteria:** Successfully connects to WiFi, gets IP, time syncs

#### Test 3.3: Connection Status
**Expected:**
- Status updates during connection
- Shows CONNECTED (3) when successful

**Steps:**
1. Initiate WiFi connection
2. Poll: `http://192.168.0.1/wifiConnectStatus`
3. Observe status changes:
   - 0: DISCONNECTED
   - 1: CONNECTING
   - 2: FAILED (if wrong credentials)
   - 3: CONNECTED

✅ **Pass Criteria:** Status transitions correctly

#### Test 3.4: WiFi Disconnect
**Expected:**
- Can disconnect from external WiFi
- SNTP stops
- AP remains active

**Steps:**
1. While connected to external WiFi
2. Click "Disconnect" button
3. Monitor serial output:
   ```
   I (xxx) wifi_app: Received: USER_REQUESTED_STA_DISCONNECT
   I (xxx) sntp_client: Stopping SNTP client
   ```
4. Verify still connected to ESP32_AP

✅ **Pass Criteria:** Disconnects from WiFi, SNTP stops, AP stays active

### Phase 4: Time Synchronization

#### Test 4.1: SNTP Client
**Expected:**
- Time syncs after WiFi connection
- Displays Germany timezone

**Steps:**
1. Connect ESP32 to internet-connected WiFi
2. Wait 5-10 seconds for sync
3. Fetch: `http://192.168.0.1/localTime.json`
4. Verify formatted time in Germany timezone
5. Check serial monitor:
   ```
   I (xxx) sntp_client: Time synchronized
   ```

✅ **Pass Criteria:** Time displays in correct timezone format

### Phase 5: DNS Server

#### Test 5.1: Captive Portal Detection
**Expected:**
- Phone detects captive portal
- DNS queries return ESP32 IP

**Steps:**
1. Connect phone to ESP32_AP
2. Phone should show "Sign in to network" notification
3. Monitor serial output:
   ```
   D (xxx) dns_server: DNS query responded
   ```

✅ **Pass Criteria:** Captive portal detected, DNS responds

### Phase 6: OTA Update (Framework)

#### Test 6.1: OTA Status
**Expected:**
- Returns current firmware info

**Steps:**
1. POST to: `http://192.168.0.1/OTAstatus`
2. Verify response:
   ```json
   {
     "ota_update_status": 0,
     "compile_date": "Dec 24 2025",
     "compile_time": "15:30:00"
   }
   ```

✅ **Pass Criteria:** OTA status returns firmware info

#### Test 6.2: OTA Upload (Not Yet Implemented)
**Expected:**
- Returns 501 Not Implemented

**Steps:**
1. POST firmware file to: `http://192.168.0.1/OTAupdate`
2. Verify 501 response

✅ **Pass Criteria:** Returns appropriate error (implementation pending)

## Troubleshooting

### Issue: AP Not Visible
**Symptoms:** Can't see ESP32_AP in WiFi list

**Solutions:**
1. Check serial monitor for "Access Point started"
2. Verify WiFi channel not congested
3. Try changing `WIFI_AP_CHANNEL` in app_wifi.h
4. Restart ESP32

### Issue: Can't Connect to AP
**Symptoms:** Connection fails or times out

**Solutions:**
1. Verify password is "password" (case-sensitive)
2. Check max connections not exceeded (5 clients)
3. Look for errors in serial monitor
4. Restart ESP32 and try again

### Issue: Web Page Not Loading
**Symptoms:** Browser shows "Can't reach this page"

**Solutions:**
1. Verify connected to ESP32_AP
2. Check IP address is 192.168.0.x
3. Try http://192.168.0.1 explicitly
4. Check HTTP server started in serial monitor
5. Clear browser cache

### Issue: Sensor Readings Not Updating
**Symptoms:** Temperature/humidity show old values or errors

**Solutions:**
1. Check DHT22 wiring (GPIO pin correct)
2. Verify sensor power (3.3V or 5V)
3. Check serial monitor for DHT read errors
4. Sensor needs 2 seconds between reads
5. Try different GPIO pin if issues persist

### Issue: WiFi Connection Fails
**Symptoms:** Can't connect to home WiFi

**Solutions:**
1. Verify SSID and password correct
2. Check WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
3. Ensure router not at max clients
4. Check router security (WPA2 supported)
5. Monitor serial for disconnect reason code

### Issue: Time Not Syncing
**Symptoms:** Local time shows "not available"

**Solutions:**
1. Verify WiFi connected to internet
2. Check firewall allows NTP (UDP port 123)
3. Wait 10-15 seconds for first sync
4. Check serial for SNTP errors
5. Try different NTP server in config.h

### Issue: Build Errors
**Symptoms:** Compilation fails

**Common Fixes:**
1. Clean build: `idf.py fullclean && idf.py build`
2. Check ESP-IDF version (v5.x required)
3. Verify all component dependencies
4. Check CMakeLists.txt syntax
5. Update ESP-IDF: `git pull` in esp-idf directory

### Issue: Flash Errors
**Symptoms:** Can't flash firmware

**Solutions:**
1. Check USB cable (data cable, not charge-only)
2. Hold BOOT button while flashing
3. Try different USB port
4. Verify correct COM port
5. Check partition table size

## Performance Monitoring

### Memory Usage
Monitor heap in serial output:
```
I (xxx) app_coordinator: heap_free: 123456, heap_min: 98765
```

**Healthy Values:**
- Free heap: > 80KB
- Min heap: > 60KB

**If Low:**
- Reduce task stack sizes
- Limit WebSocket clients
- Optimize buffer sizes

### CPU Usage
Monitor task states:
```
idf.py monitor
# Press 't' for task list
```

### Network Performance
- AP clients: Check connection count
- HTTP requests: Monitor response times
- WiFi signal: Check RSSI values

## Next Steps

1. **Complete Remaining Features:**
   - Implement WebSocket for real-time updates
   - Add OTA upload handler
   - Implement config backup/restore
   - Modernize web interface (remove jQuery)

2. **Enhance Web Interface:**
   - Add WiFi scanner UI
   - Add system status display
   - Add progress bars for OTA
   - Improve mobile responsiveness

3. **Add Security:**
   - Implement OTA signature verification
   - Add HTTPS support
   - Implement authentication
   - Rate limiting

4. **Production Readiness:**
   - Add error recovery
   - Implement watchdog timers
   - Add logging levels
   - Create user documentation

## Success Checklist

- [ ] ESP32 boots and creates AP
- [ ] Can connect to ESP32_AP
- [ ] Web interface loads
- [ ] DHT sensor readings display
- [ ] LED responds to humidity
- [ ] WiFi scan works
- [ ] Can connect to external WiFi
- [ ] Time syncs via SNTP
- [ ] Captive portal triggers
- [ ] System status displays
- [ ] Can disconnect from WiFi
- [ ] All components running stable

## Support

For issues or questions:
1. Check serial monitor output
2. Review troubleshooting section
3. Check ESP-IDF documentation
4. Review component source code
5. Check GitHub issues (if applicable)

---

**Happy Testing! 🚀**

