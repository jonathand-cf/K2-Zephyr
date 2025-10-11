// Zephyr RTOS kernel and logging headers
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
// Zephyr networking stack headers for socket operations
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
// Standard C library headers for string manipulation and I/O
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>

// Include LED control header for visual feedback
#include "led.h"
#include "control.h"

#include "bitmask.h"
#include "crc32.h"

// Declare this module for logging purposes
LOG_MODULE_DECLARE(k2_app);

// Network configuration constants
#define UDP_PORT 12345          // Port number for UDP server to listen on
#define RECV_BUFFER_SIZE 64     // Buffer size for incoming UDP messages

// Packet structure definition
typedef struct {
    uint32_t sequence;  // Sequence number
    uint64_t payload;   // Payload data
    uint32_t crc32;     // CRC32 checksum
} __attribute__((packed)) udp_packet_t;

_Static_assert(sizeof(udp_packet_t) == 16, "udp_packet_t must be 16 bytes");

// Static IP configuration - customize these for your network
#define STATIC_IP_ADDR "192.168.1.100"   // Device's static IP address
#define STATIC_NETMASK "255.255.255.0"   // Subnet mask
#define STATIC_GATEWAY "192.168.1.1"     // Default gateway address

// Network management callback structure for handling interface events
static struct net_mgmt_event_callback mgmt_cb;
bool network_ready = false;  // Flag to track network interface status

static inline uint64_t host_to_net_64(uint64_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(value);
#else
    return value;
#endif
}

// UDP socket and thread management variables
int udp_sock = -1;                                    // UDP socket file descriptor
K_THREAD_STACK_DEFINE(udp_thread_stack, 2048);      // Stack space for UDP thread
struct k_thread udp_thread_data;                     // Thread control block



/**
 * Network management event handler - called when network interface events occur
 * @param cb: Callback structure (unused)
 * @param mgmt_event: Type of network management event
 * @param iface: Network interface that generated the event
 */
static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event, struct net_if *iface)
{
    // Check if network interface came up
    if (mgmt_event == NET_EVENT_IF_UP) {
        LOG_INF("Network interface is up - network is ready");
        network_ready = true;  // Set flag to indicate network is available
    } 
    // Check if network interface went down
    else if (mgmt_event == NET_EVENT_IF_DOWN) {
        LOG_WRN("Network interface is down");
        network_ready = false;  // Clear network ready flag
    }
}

/**
 * Parse IPv4 address string into binary format
 * @param str: String representation of IP address (e.g., "192.168.1.100")
 * @param addr: Output structure to store parsed address
 * @return: 0 on success, negative error code on failure
 */
static inline int parse_ipv4_addr(const char *str, struct in_addr *addr) // Added: inline
{
    unsigned int a, b, c, d;
    
    // Validate and parse the IPv4 address string
    if (sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || 
        a > 255 || b > 255 || c > 255 || d > 255) {
        return -EINVAL;
    }
    
    addr->s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
    return 0;
}

/**
 * Configure static IP settings for the network interface
 * @param iface: Network interface to configure
 * @return: 0 on success, negative error code on failure
 */
static int configure_static_ip(struct net_if *iface)
{
    struct in_addr addr;     // IP address structure
    struct in_addr netmask;  // Netmask structure
    struct in_addr gateway;  // Gateway address structure
    int ret;

    // Configure IP address
    ret = parse_ipv4_addr(STATIC_IP_ADDR, &addr);
    if (ret < 0) {
        LOG_ERR("Invalid IP address format: %s", STATIC_IP_ADDR);
        return -EINVAL;
    }
    // Add the IP address to the interface with manual configuration
    net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

    // Configure netmask (subnet mask)
    ret = parse_ipv4_addr(STATIC_NETMASK, &netmask);
    if (ret < 0) {
        LOG_ERR("Invalid netmask format: %s", STATIC_NETMASK);
        return -EINVAL;
    }
    // Set the netmask for the configured IP address
    net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask);

    // Configure gateway (default route)
    ret = parse_ipv4_addr(STATIC_GATEWAY, &gateway);
    if (ret < 0) {
        LOG_ERR("Invalid gateway format: %s", STATIC_GATEWAY);
        return -EINVAL;
    }
    // Set the default gateway for the interface
    net_if_ipv4_set_gw(iface, &gateway);

    // Log the complete static IP configuration for verification
    LOG_INF("Static IP configuration:");
    LOG_INF("  IP: %s", STATIC_IP_ADDR);
    LOG_INF("  Netmask: %s", STATIC_NETMASK);
    LOG_INF("  Gateway: %s", STATIC_GATEWAY);

    return 0;  // Configuration successful
}

/**
 * Initialize the network subsystem with static IP configuration
 * This function sets up the network interface and applies static IP settings
 */
void network_init(void)
{
    struct net_if *iface;  // Network interface handle
    int ret;

    LOG_INF("Initializing network with static IP...");

    // Get the default network interface
    iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No network interface found");
        return;  // Cannot proceed without a network interface
    }

    // Initialize network management event callback to monitor interface status
    net_mgmt_init_event_callback(&mgmt_cb, net_mgmt_event_handler,
                                 NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
    // Register the callback with the network management subsystem
    net_mgmt_add_event_callback(&mgmt_cb);

    // Apply static IP configuration to the interface
    ret = configure_static_ip(iface);
    if (ret < 0) {
        LOG_ERR("Failed to configure static IP: %d", ret);
        return;  // Configuration failed
    }

    // Bring the network interface up (activate it)
    net_if_up(iface);
    
    // Allow some time for the interface to become operational
    k_sleep(K_MSEC(200));
    network_ready = true;  // Mark network as ready for use
    
    LOG_INF("Static IP configuration complete");
}


/**
 * Convert 64-bit value from network byte order to host byte order
 * @param value: 64-bit value in network byte order
 * @return: 64-bit value in host byte order
 */
static inline uint64_t net_to_host_64(uint64_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(value);
#else
    return value;
#endif
}


static int send_udp_packet(int sock, const struct sockaddr_in *to,
                           uint32_t seq_host, uint64_t payload_host)
{
    udp_packet_t out = {0};

    // Fill in network order for sequence/payload
    out.sequence = htonl(seq_host);
    out.payload  = host_to_net_64(payload_host);

    uint32_t crc = crc32_ieee(&out, offsetof(udp_packet_t, crc32));
    out.crc32 = htonl(crc);

    return zsock_sendto(sock, &out, sizeof(out), 0,
                        (const struct sockaddr *)to, sizeof(*to));
}

/**
 * UDP server thread function - handles incoming UDP messages
 */
void udp_server_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1); ARG_UNUSED(arg2); ARG_UNUSED(arg3);

    struct sockaddr_in bind_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int ret;

    while (!network_ready) {
        k_sleep(K_MSEC(100));
    }

    udp_sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock < 0) {
        LOG_ERR("Failed to create UDP socket: %d", udp_sock);
        return;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(UDP_PORT);

    ret = zsock_bind(udp_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
    if (ret < 0) {
        LOG_ERR("Failed to bind UDP socket: %d", ret);
        zsock_close(udp_sock);
        return;
    }

    LOG_INF("UDP server ready on port %d", UDP_PORT);

    while (1) {
        udp_packet_t packet;
        client_addr_len = sizeof(client_addr);

        ret = zsock_recvfrom(udp_sock, &packet, sizeof(packet), 0,
                             (struct sockaddr *)&client_addr, &client_addr_len);

        if (ret == sizeof(udp_packet_t)) {
            uint32_t recv_sequence = ntohl(packet.sequence);
            uint64_t recv_payload  = net_to_host_64(packet.payload);
            uint32_t recv_crc      = ntohl(packet.crc32);

            // Recompute CRC over sequence+payload (network order)
            uint32_t calc_crc = crc32_ieee(&packet, offsetof(udp_packet_t, crc32));

            if (calc_crc == recv_crc) {
                // Forward to control as before
                rov_send_command(recv_sequence, recv_payload);

                // Reply with current bitmask
                uint64_t current_bm = bm_get_current();
                (void)send_udp_packet(udp_sock, &client_addr, recv_sequence, current_bm);
            } else {
                LOG_ERR("CRC MISMATCH - drop (seq=%u)", recv_sequence);
            }
        } else if (ret < 0) {
            LOG_ERR("UDP recv error: %d", ret);
            k_sleep(K_MSEC(100));
        } else {
            LOG_WRN("Wrong packet size: got %d", ret);
        }
    }
}

/**
 * Start the UDP server by creating and launching the server thread
 * This function creates a new thread that will handle all UDP server operations
 */
void udp_server_start(void)
{
    k_tid_t thread_id;
    
    // Create and start the UDP server thread
    thread_id = k_thread_create(&udp_thread_data,
                               udp_thread_stack,
                               K_THREAD_STACK_SIZEOF(udp_thread_stack),
                               udp_server_thread,
                               NULL, NULL, NULL,
                               K_PRIO_COOP(7),
                               0,
                               K_NO_WAIT);
    
    if (thread_id != NULL) {
        LOG_INF("UDP server thread created successfully");
    } else {
        LOG_ERR("Failed to create UDP server thread");
    }
}