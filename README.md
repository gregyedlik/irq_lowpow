# irq_lowpow
 
This project was born of the following ChatGPI o1 prompt:

```
I would like to ask for your help writing me a software to solve the low power sleep with watchdog and interrupt awake problem.

I want the software to run on a Raspberry Pi Pico, with the RP2040 microcontroller. The Pico has a button attached to the pin 24. The software should be in C, using the Pico SDK.

I want software that:
* Sets up a watchdog for 30 seconds.
* Sets up the GPIO pin 13 as input, where a button is connected. The microcontroller should pull the pin low. When the button is pressed, it will pull the pin high.
* Sets up an interrupt that is able to pull it out from a low-power sleep mode. The interrupt should be attached to the rising edge of the GPIO 13.
* Goes to a low power sleep mode

There should be two ways to get out of that low power sleep mode:
* Reset of the microcontroller by the watchdog
* Interrupt

If the interrupt fires, it should leave the program in a loop, waiting on the pressing of a button would pat the watchdog.

The only way out of that loop should be the watchdog resetting the microcontroller.

So basically there are two states the microcontroller can be in:
* Sleeping, waiting for the interrupt or the watchdog
* Being awake waiting for the button presses or the watchdog

It is key to have a sleep mode that is as-low-power-consumption as possible, but that does not block the watchdog from working.

Once waken up from the low-power-sleep-mode, the microcontroller should be restored to full frequency as it was before entering the sleep mode. There should be functionality to show this.
```

It does have various problems as of the initial commit:
1. The watchdog goes off after only abouy 8 seconds, not 30
