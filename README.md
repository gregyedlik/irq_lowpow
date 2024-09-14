# irq_lowpow

This project was born to solve the low-power-sleep with watchdog and interrupt-awake problem on the RP2040 with a Raspberry Pi Pico.

I want software that:
* Sets up a watchdog.
* Sets up an interrupt that can wake the microcontroller from a low-power-sleep mode.
* Puts the microcontroller into a low-power-sleep mode.

There should be two ways to wake the microcontroller from that low-power-sleep mode:
* A reset of the microcontroller by the watchdog.
* An interrupt.

If the interrupt fires, the program should enter a loop, waiting for a button press to pat the watchdog.

The only way out of that loop should be the watchdog resetting the microcontroller.

So, there are three states the microcontroller can be in:
* Initialization.
* Low-power-sleep, waiting for an interrupt or the watchdog.
* Light sleep, waiting for button presses or the watchdog.

The goal is to have a sleep mode with the lowest power consumption possible that does not block the watchdog from working.

Once woken from low-power-sleep mode, the microcontroller should be restored to full frequency, as it was before entering sleep mode. There should be functionality to confirm this.

## Known problem

Based on current measurements, it seems not everything is restored. This is still a mystery to solve.
