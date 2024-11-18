#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdlib.h>   // For strtoul
#include <netinet/in.h>
#include <sys/socket.h>

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
    CMD_SEND_IP = 0x20,  // New command ID for sending IP
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

// Function to get IP address of a specific interface
int get_ip_address(char *buffer, size_t buflen) {
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "eth0") == 0) { // Replace "eth0" if needed
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            strncpy(buffer, inet_ntoa(addr->sin_addr), buflen);
            buffer[buflen - 1] = '\0';  // Ensure null termination
            break;
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}

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

int main() {
    while(1){
        int ret = 0;
        int fd = open(RTOS_CMDQU_DEV_NAME, O_RDWR);
        if (fd <= 0) {
            printf("open failed! fd = %d\n", fd);
            return 0;
        }

        struct cmdqu_t cmd = {0};

        // Send IP Address
        char ip_address[INET_ADDRSTRLEN] = {0};
        if (get_ip_address(ip_address, sizeof(ip_address)) == 0) {
            cmd.ip_id = 0;
            cmd.cmd_id = CMD_SEND_IP;
            cmd.resv.mstime = 100;
            cmd.param_ptr = (unsigned int)strtoul(ip_address, NULL, 10);

            ret = ioctl(fd, RTOS_CMDQU_SEND_WAIT, &cmd);
            if (ret < 0) {
                printf("ioctl error while sending IP!\n");
                close(fd);
                return 0;
            }
            printf("IP Address sent: %s\n", ip_address);
        } else {
            printf("Failed to get IP address.\n");
        }

        // Send CPU Temperature
        int cpu_temp = get_cpu_temperature();
        if (cpu_temp >= 0) {
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

        close(fd);
        sleep(1);
    }
    return 0;
}
