#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>

enum SYSTEM_CMD_TYPE {
    CMDQU_SEND = 1,
    CMDQU_SEND_WAIT,
    CMDQU_SEND_WAKEUP,
};

#define RTOS_CMDQU_DEV_NAME "/dev/cvi-rtos-cmdqu"
#define RTOS_CMDQU_SEND                         _IOW('r', CMDQU_SEND, unsigned long)
#define RTOS_CMDQU_SEND_WAIT                    _IOW('r', CMDQU_SEND_WAIT, unsigned long)
#define RTOS_CMDQU_SEND_WAKEUP                  _IOW('r', CMDQU_SEND_WAKEUP, unsigned long)

enum SYS_CMD_ID {
    CMD_SEND_IP_1 = 0x20,  // New command ID for sending IP
    CMD_SEND_IP_2,         // New command ID for sending IP
    CMD_SEND_IP_3,         // New command ID for sending IP
    CMD_SEND_IP_4,         // New command ID for sending IP
    CMD_SEND_CONNECTED_DEVICES, // New command ID for sending connected device
    CMD_SEND_TEMP,       // New command ID for sending temperature
    SYS_CMD_INFO_LIMIT,
};

struct valid_t {
    unsigned char linux_valid;
    unsigned char rtos_valid;
} __attribute__((packed));

typedef union resv_t {
    struct valid_t valid;
    unsigned short mstime; // 0 : noblock, -1 : block infinite
} resv_t;

typedef struct cmdqu_t cmdqu_t;
/* cmdqu size should be 8 bytes because of mailbox buffer size */
struct cmdqu_t {
    unsigned char ip_id;
    unsigned char cmd_id : 7;
    unsigned char block : 1;
    union resv_t resv;
    unsigned int  param_ptr;
} __attribute__((packed)) __attribute__((aligned(0x8)));

// Function to get CPU temperature
int get_cpu_temperature() {
    int temperature;
    FILE *tempFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");

    if (tempFile == NULL) {
        perror("Failed to read temperature");
        return -1;
    }

    fscanf(tempFile, "%d", &temperature);
    fclose(tempFile);

    return temperature / 1000;  // Assuming temperature is in millidegree Celsius
}

// Function to get the local IP and MAC address
void get_local_ip_mac(const char *iface, uint8_t *mac, uint8_t *ip) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;

    // Get IP address
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    memcpy(ip, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, 4);

    // Get MAC address
    ioctl(fd, SIOCGIFHWADDR, &ifr);
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

    close(fd);
}

// Function to send ARP request
void send_arp_request(int sock, const char *iface, uint8_t *src_mac, uint8_t *src_ip, uint8_t *target_ip) {
    uint8_t buffer[42];
    struct ether_header *eth = (struct ether_header *) buffer;
    struct ether_arp *arp = (struct ether_arp *) (buffer + ETH_HLEN);

    // Construct Ethernet header
    memset(eth->ether_dhost, 0xff, 6); // Broadcast
    memcpy(eth->ether_shost, src_mac, 6);
    eth->ether_type = htons(ETH_P_ARP);

    // Construct ARP request
    arp->arp_hrd = htons(ARPHRD_ETHER);            // Hardware type: Ethernet
    arp->arp_pro = htons(ETH_P_IP);                // Protocol type: IPv4
    arp->arp_hln = ETHER_ADDR_LEN;                 // Hardware address length: 6
    arp->arp_pln = 4;                              // Protocol address length: 4
    arp->arp_op = htons(ARPOP_REQUEST);            // Opcode: request
    memcpy(arp->arp_sha, src_mac, 6);              // Sender MAC
    memcpy(arp->arp_spa, src_ip, 4);               // Sender IP
    memset(arp->arp_tha, 0x00, 6);                 // Target MAC: unknown
    memcpy(arp->arp_tpa, target_ip, 4);            // Target IP

    // Set socket address
    struct sockaddr_ll socket_address;
    socket_address.sll_ifindex = if_nametoindex(iface);
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, eth->ether_dhost, 6);

    // Send the packet
    if (sendto(sock, buffer, 42, 0, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0) {
        perror("sendto failed");
    }
}

// Function to scan the network and print connected devices
int scan_network(const char *iface) {
    int sock;
    int device_count = 0;
    struct timeval timeout;
    timeout.tv_sec = 1; // Set timeout in seconds
    timeout.tv_usec = 0;
    
    uint8_t src_mac[6], src_ip[4];
    uint8_t target_ip[4];

    // Get local IP and MAC address
    get_local_ip_mac(iface, src_mac, src_ip);

    // Create raw socket
    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    printf("Scanning network on interface %s...\n", iface);

    // Iterate over IPs in the subnet (assumes /24 subnet)
    for (int i = 1; i < 255; i++) {
        memcpy(target_ip, src_ip, 3);
        target_ip[3] = i;
        send_arp_request(sock, iface, src_mac, src_ip, target_ip);
    }

    // Receive ARP replies
    uint8_t buffer[ETH_FRAME_LEN];
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    while (1) {
        // Set timeout
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1)
        {
            perror("select failed");
            break;
        }
        else if (ret == 0)
        {
            printf("Timeout\n");
            break;
        }
        

        int len = recvfrom(sock, buffer, ETH_FRAME_LEN, 0, &saddr, &saddr_len);
        if (len < 0) {
            perror("recvfrom failed");
            break;
        }

        struct ether_header *eth = (struct ether_header *) buffer;
        if (ntohs(eth->ether_type) == ETH_P_ARP) {
            struct ether_arp *arp = (struct ether_arp *) (buffer + ETH_HLEN);
            if (ntohs(arp->arp_op) == ARPOP_REPLY) {
                // printf("Device found: IP = %d.%d.%d.%d, MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
                //     arp->arp_spa[0], arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3],
                //     arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2],
                //     arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5]);
                    device_count++;
            }
        }
    }

    // Close the socket
    close(sock);
    return device_count;
}

int main() {
    int connected_devices;
    int ret;
    uint8_t src_mac[6], src_ip[4];

    while(1) {

        int fd = open(RTOS_CMDQU_DEV_NAME, O_RDWR);
        if (fd <= 0) {
            printf("open failed! fd = %d\n", fd);
            return 0;
        }

        struct cmdqu_t cmd = {0};

        // Send CPU Temperature
        int cpu_temp = get_cpu_temperature();
        if (cpu_temp >= 0) {
            cmd.ip_id = 0;
            cmd.resv.mstime = 100;
            cmd.cmd_id = CMD_SEND_TEMP;
            cmd.param_ptr = (unsigned int)cpu_temp;

            ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
            if (ret < 0) {
                printf("ioctl error while sending temperature!\n");
                close(fd);
                return 0;
            }
            printf("CPU Temperature sent: %d°C\n", cpu_temp);
        } else {
            printf("Failed to read CPU temperature.\n");
        }
        // sleep(1);
        // Get local IP and MAC address
        get_local_ip_mac("eth0", src_mac, src_ip);

        // Send IP address
        cmd.ip_id = 0;
        cmd.cmd_id = CMD_SEND_IP_1;
        cmd.resv.mstime = 100;
        cmd.param_ptr = (unsigned int)src_ip[0];

        ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
        if (ret < 0) {
            printf("ioctl error while sending IP!\n");
            close(fd);
            return 0;
        }
        // sleep(1);
        cmd.ip_id = 0;
        cmd.cmd_id = CMD_SEND_IP_2;
        cmd.resv.mstime = 100;
        cmd.param_ptr = (unsigned int)src_ip[1];

        ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
        if (ret < 0) {
            printf("ioctl error while sending IP!\n");
            close(fd);
            return 0;
        }
        // sleep(1);
        cmd.ip_id = 0;
        cmd.cmd_id = CMD_SEND_IP_3;
        cmd.resv.mstime = 100;
        cmd.param_ptr = (unsigned int)src_ip[2];

        ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
        if (ret < 0) {
            printf("ioctl error while sending IP!\n");
            close(fd);
            return 0;
        }
        // sleep(1);
        cmd.ip_id = 0;
        cmd.cmd_id = CMD_SEND_IP_4;
        cmd.resv.mstime = 100;
        cmd.param_ptr = (unsigned int)src_ip[3];

        ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
        if (ret < 0) {
            printf("ioctl error while sending IP!\n");
            close(fd);
            return 0;
        }
        printf("> IP Address sent: %d.%d.%d.%d\n", src_ip[0], src_ip[1], src_ip[2], src_ip[3]);

        // sleep(1);
        connected_devices = scan_network("eth0");
        cmd.ip_id = 0;
        cmd.cmd_id = CMD_SEND_CONNECTED_DEVICES;
        cmd.resv.mstime = 100;
        cmd.param_ptr = (unsigned int)connected_devices;

        ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
        if (ret < 0) {
            printf("ioctl error while sending IP!\n");
            close(fd);
            return 0;
        }
        printf("> Connected devices: %d\n", connected_devices);
        close(fd);
    }



    return 0;
}
