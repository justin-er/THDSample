The ESP32 should start its Access Point so that other devices can connect to it.
This enables users to access information e.g., sensor data, device info.,
connection status/information, user option to connect to and disconnect from
an AP, display local time, etc..
The WiFi application will start an HTTP Server (created in the next section), which
will support a web page (to do all the stuff mentioned above).
The application will contain a FreeRTOS task that accepts FreeRTOS Queue
messages (xQueueCreate, xQueueSend and xQueueReceive) for event coordination.

Connecting Device: 
- Becomes “station” of ESP32’s SoftAP
when connected.
- DHCP service from the ESP32’s SoftAP will dynamically assign an IP to your device.
- Interact with the ESP32 via the webpage.

ESP32 SoftAP/Station
• AP/STA combination mode.
• We assign an IP to the SoftAP; the
interface of the ESP32 is statically
configured.
• DHCP server dynamically assigns an
IP for connecting stations.
• We set a maximum number of
stations allowed to connect.

Connecting Device
• When the ESP32 connects to an AP,
local time can be obtained utilizing
SNTP (if the AP is connected to the
internet).
• DHCP service from the AP, will
dynamically assign an IP to the ESP32
in our application.