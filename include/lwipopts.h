#ifndef ROBOT_LWIPOPTS_H
#define ROBOT_LWIPOPTS_H

/* One core, cooperative polling, raw lwIP API; no OS or socket layer. */
/* lwIP is the small TCP/IP networking stack used by the Pico SDK.
   These are compile-time library options, not your WiFi credentials.
   For enable/disable options, 1 means enabled and 0 means disabled. */
#define NO_SYS 1                 /* Run without an operating-system networking thread. */
#define LWIP_SOCKET 0            /* Do not build the desktop-style sockets API. */
#define LWIP_NETCONN 0           /* Do not build the threaded netconn API either. */
#define MEM_ALIGNMENT 4         /* Align lwIP heap allocations on 4-byte boundaries. */
#define MEM_SIZE 16384           /* Reserve 16 KiB for the lwIP heap. */
#define MEMP_NUM_TCP_SEG 32      /* Pool capacity for TCP segment descriptors. */
#define MEMP_NUM_ARP_QUEUE 10    /* Queue entries while resolving local network addresses. */
#define PBUF_POOL_SIZE 24       /* Number of buffers in the packet-buffer pool. */
#define LWIP_ARP 1              /* Resolve IPv4 addresses to local link-layer addresses. */
#define LWIP_ETHERNET 1         /* Support Ethernet-format frames used by this WiFi stack. */
#define LWIP_ICMP 1             /* Support IPv4 control messages such as ping. */
#define LWIP_IPV4 1             /* Enable addresses such as 192.168.1.20. */
#define LWIP_DHCP 1             /* Request the Pico's IP configuration from the router. */
#define LWIP_DNS 0              /* No hostname lookup: configure a numeric broker IP. */
#define LWIP_TCP 1              /* Reliable byte-stream transport used by MQTT. */
#define LWIP_UDP 1              /* Datagram transport needed by services such as DHCP. */
#define TCP_MSS 1460            /* Maximum TCP data bytes per segment for this setup. */
#define TCP_WND (8 * TCP_MSS)    /* TCP receive window in bytes. */
#define TCP_SND_BUF (8 * TCP_MSS) /* TCP send-buffer budget in bytes. */
#define TCP_SND_QUEUELEN 32      /* Maximum number of packet buffers in the TCP send queue. */
#define LWIP_NETIF_STATUS_CALLBACK 1 /* Enable notifications about interface status changes. */
#define LWIP_NETIF_LINK_CALLBACK 1   /* Enable notifications about link up/down changes. */
#define LWIP_NETIF_HOSTNAME 1        /* Allow a network interface to have a hostname. */
#define LWIP_NETIF_TX_SINGLE_PBUF 1  /* Request each transmitted packet in one buffer. */
#define LWIP_TCP_KEEPALIVE 1         /* Compile support for TCP keepalive options. */
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1) /* Internal timers + MQTT timer. */
#define MQTT_OUTPUT_RINGBUF_SIZE 4096 /* Bytes in MQTT's circular outgoing packet buffer. */
#define MQTT_REQ_MAX_IN_FLIGHT 8      /* Maximum outstanding MQTT requests awaiting completion. */
#define MQTT_CONNECT_TIMOUT 10       /* MQTT connection timeout in seconds; spelling is lwIP's. */
#define MQTT_REQ_TIMEOUT 10          /* MQTT request timeout in seconds. */

#endif
