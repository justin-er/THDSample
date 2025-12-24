#ifndef CONFIG_H
#define CONFIG_H

/**
 * SNTP Timezone Configuration
 * 
 * Format: "STD offset DST,start,end"
 * 
 * Common timezones:
 *   Germany:    "CET-1CEST,M3.5.0,M10.5.0/3"
 *   US Pacific: "PST8PDT,M3.2.0/2,M11.1.0"
 *   US Eastern: "EST5EDT,M3.2.0/2,M11.1.0"
 *   UTC:        "UTC0"
 *   Tokyo:      "JST-9"
 * 
 * To change timezone: Replace the value below and rebuild
 */
#define SNTP_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define SNTP_SERVER "pool.ntp.org"

#endif // CONFIG_H
