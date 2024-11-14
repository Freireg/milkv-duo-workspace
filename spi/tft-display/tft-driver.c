#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "wiringx.h"  // Include the wiringX library

#define ST7789V_SPI_CHANNEL 0
#define SPI_SPEED   500000

// Define GPIO pins for controlling D/C and reset if needed
#define DC_PIN 17
#define RESET_PIN 27

// SPI functions
void writeCommand(uint8_t cmd);
void writeData(uint8_t data);
void initDisplay(void);

void writeCommand(uint8_t cmd) {
    uint8_t data[1] = {cmd};
    wiringXSPIDataRW(ST7789V_SPI_CHANNEL, data, 1);
}

void writeData(uint8_t data) {
    uint8_t dataArray[1] = {data};
    wiringXSPIDataRW(ST7789V_SPI_CHANNEL, dataArray, 1);
}

void initDisplay(void) {
    // 1. Software Reset
    writeCommand(0x01); // Software reset command
    usleep(150000);      // Wait for 150ms

    // 2. Sleep Out
    writeCommand(0x11); // Sleep Out command
    usleep(120000);      // Wait for 120ms

    // 3. Frame Rate Control
    writeCommand(0xB2); // PORCH control
    writeData(0x0C);    // Frame control values for front porch and back porch
    writeData(0x0C);
    writeData(0x00);
    writeData(0x33);
    writeData(0x33);

    // 4. Gate Control
    writeCommand(0xB7); // Gate control
    writeData(0x35);

    // 5. VCOM Setting
    writeCommand(0xBB); // VCOM setting
    writeData(0x19);    // Recommended VCOM setting

    // 6. LCM Control
    writeCommand(0xC0); // LCM control
    writeData(0x2C);

    // 7. VDV and VRH Command Enable
    writeCommand(0xC2); // Command Enable
    writeData(0x01);
    writeData(0xFF);

    // 8. VRH Set
    writeCommand(0xC3);
    writeData(0x11);

    // 9. VDV Set
    writeCommand(0xC4);
    writeData(0x20);

    // 10. Frame Rate Control in Normal Mode
    writeCommand(0xC6);
    writeData(0x0F);

    // 11. Power Control
    writeCommand(0xD0);
    writeData(0xA4);
    writeData(0xA1);

    // 12. Memory Data Access Control
    writeCommand(0x36); // Memory Data Access Control
    writeData(0x00);    // Row/Column exchange settings (can adjust orientation)

    // 13. Color Format
    writeCommand(0x3A); // Pixel format set
    writeData(0x05);    // 16-bit color

    // 14. Display On
    writeCommand(0x29); // Display on
    usleep(100000);     // Wait for display on
}

// Set a rectangular area on the display
void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(0x2A); // Column Address Set
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);

    writeCommand(0x2B); // Row Address Set
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);

    writeCommand(0x2C); // Memory Write
}

void fillDisplay(uint16_t color) {
    setAddressWindow(0, 0, 239, 239); // For a 240x240 display

    uint8_t highByte = color >> 8;
    uint8_t lowByte = color & 0xFF;

    for (uint32_t i = 0; i < 240 * 240; i++) {
        writeData(highByte);
        writeData(lowByte);
    }
}

void clearDisplay() {
    fillDisplay(0x0000); // Black color in RGB 5-6-5
}


int main() {
    // Initialize wiringX
    if (wiringXSetup("milkv_duo", NULL) == -1) {
        fprintf(stderr, "Failed to initialize wiringX\n");
        return -1;
    }

    // Set up SPI
    int spi_fd = wiringXSPISetup(ST7789V_SPI_CHANNEL, SPI_SPEED);
    if (spi_fd == -1) {
        fprintf(stderr, "Failed to set up SPI\n");
        return -1;
    }
    // Initialize Display
    initDisplay();

    // Fill display with blue as an example
    fillDisplay(0x001F); // Blue color
    sleep(2);             // Wait for 2 seconds
    // Clear the display
    clearDisplay();

    return 0;
}
