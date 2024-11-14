#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "wiringx.h"  // Include the wiringX library

#define SPI_CHANNEL 0
#define SPI_SPEED   500000

// Define GPIO pins for controlling D/C and reset if needed
#define DC_PIN 17
#define RESET_PIN 27

// SPI functions
void sendCommand(uint8_t cmd);
void sendData(uint8_t data);
void initDisplay(void);

int main() {
    // Initialize wiringX
    if (wiringXSetup("milkv_duo", NULL) == -1) {
        fprintf(stderr, "Failed to initialize wiringX\n");
        return -1;
    }

    // Set up SPI
    int spi_fd = wiringXSPISetup(SPI_CHANNEL, SPI_SPEED);
    if (spi_fd == -1) {
        fprintf(stderr, "Failed to set up SPI\n");
        return -1;
    }

    // Set up GPIO pins for D/C and Reset
    pinMode(DC_PIN, PINMODE_OUTPUT);
    pinMode(RESET_PIN, PINMODE_OUTPUT);

    // Initialize display
    initDisplay();

    // Example usage: clear screen (send a fill color to all pixels)
    sendCommand(0x2C);  // Memory write command
    for (int i = 0; i < 240 * 240; i++) {
        sendData(0xFF); // Red component
        sendData(0x00); // Green component
        sendData(0x00); // Blue component
    }

    // Close SPI
    // wiringXClose();

    return 0;
}

void sendCommand(uint8_t cmd) {
    digitalWrite(DC_PIN, 0);  // Set D/C to 0 for command
    wiringXSPIDataRW(SPI_CHANNEL, &cmd, 1);
}

void sendData(uint8_t data) {
    digitalWrite(DC_PIN, 1);  // Set D/C to 1 for data
    wiringXSPIDataRW(SPI_CHANNEL, &data, 1);
}

void initDisplay(void) {
    // Hardware reset
    digitalWrite(RESET_PIN, 0);
    usleep(50000);  // Delay for 50 ms
    digitalWrite(RESET_PIN, 1);
    usleep(50000);

    // Initialization commands
    sendCommand(0x01); // Software reset
    usleep(150000);    // Delay 150 ms

    sendCommand(0x11); // Sleep out
    usleep(500000);    // Delay 500 ms

    sendCommand(0x3A); // Set color mode
    sendData(0x05);    // 16-bit color

    sendCommand(0x36); // Memory data access control
    sendData(0x00);    // Default orientation

    sendCommand(0x29); // Display ON
    usleep(100000);    // Delay 100 ms
}
