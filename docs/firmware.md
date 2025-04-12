# Display Notes

## Goals
- We want to use Rust to write the firmware for the e-ink display.


## Specifications

### Peripherals
- Bluetooth
- SPI
    - e-ink display
    - Ultrasonic sensors (TODO)

We are using the `NRF52840` dev board. We have a few options to write the code:
- [NRF SDK](https://github.com/nrfconnect/sdk-nrf) which is in `C`.
- [embassy-rs](https://github.com/embassy-rs/nrf-softdevice) but it is a wrapper on a closed source binary. 

We have chosen the NRF SDK toolchain. Using the VSCode extension, the version we are using is `2.9.1`.