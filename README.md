# MilkV Embedded Linux Development Workspace

![MilkV Board](milkv-board-image.jpg)

## Table of Contents

- [Introduction](#introduction)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Setting Up the Workspace](#setting-up-the-workspace)
- [Applications](#applications)
  - [SPI Examples](#spi-examples)
    - [LoRa](#lora)
  - [I2C Examples](#i2c-examples)
    - [OLED Display](#oled-display)

## Introduction

Welcome to my personal Milk-V Duo Development Workspace! This repository provides a environment for developing embedded Linux applications targeting the Milk-V board. 

## Getting Started

### Prerequisites

Before you begin, ensure you have the following prerequisites in place:

- A Milk-V duo board
- A Linux host machine with the required toolchain and libraries.
- Additional hardware components or modules:
  - LoRa RFM95 Module
  - TFT Display ST7789

### Setting Up the Workspace

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/your-username/milkv-embedded-linux-workspace.git
1. **Navigate to the Workspace Directory:**
    ```bash
   cd milkv-embedded-linux-workspace
1. **Install the board SDK:**
    ``` bash
    git clone https://github.com/milkv-duo/duo-examples.git
    
    cd duo-examples
    
    source envsetup.sh

## Applications
### SPI Examples
#### LoRa Module
...

### I2C Examples
#### OLED Display
...
