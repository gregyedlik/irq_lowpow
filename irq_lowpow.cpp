/***

Designed for Raspberry Pico
14/09/2024
Gijón, Spain

Note that the printf() will all go to UART. Running this via USB does not work well due to dropping the frequency much.

***/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"          // Include this header for __wfi()
#include "hardware/pll.h"           // for pll_sys and pll_init() and pll_deinit()
#include "hardware/structs/vreg_and_chip_reset.h"   // To be able to read the setup for the VREG

#define UART uart0                  // This is the default uart to use
const uint DEBOUNCE_DELAY_MS = 100; // 100 ms debounce time for the physical button / pin driving the interrupt
const uint WATCHDOG_TIMEOUT = 8388; // ms, cannot be more due to 24 bit register counting at 2 MHz
const uint LED_PIN = 25;            // General purpose LED on the Pico
const uint INT_PIN = 18;            // Interrupt pin I use on the Pico because it is next to a GND

volatile absolute_time_t last_interrupt_time;

inline static void ledON() { gpio_put(LED_PIN, true); }
inline static void ledOFF() { gpio_put(LED_PIN, false); }

void my_isr(uint gpio, uint32_t events)
{
    absolute_time_t now = get_absolute_time();
    // Check if enough time has passed since the last interrupt
    if (absolute_time_diff_us(last_interrupt_time, now) > DEBOUNCE_DELAY_MS * 1000) {
        last_interrupt_time = now;

        printf("Interrupt!\n\n");
        watchdog_update();

        gpio_acknowledge_irq(INT_PIN, GPIO_IRQ_EDGE_FALL);
    }
}

void showFreq(void)
{
    uint32_t clk_ref_freq = clock_get_hz(clk_ref);
    printf("clk_ref frequency: %u MHz\n", clk_ref_freq/1000000);

    uint32_t clk_sys_freq = clock_get_hz(clk_sys);
    printf("clk_sys frequency: %u MHz\n", clk_sys_freq/1000000);

    uint32_t clk_rtc_freq = clock_get_hz(clk_rtc);
    printf("clk_rtc frequency: %u Hz\n", clk_rtc_freq);

    uint32_t clk_peri_freq = clock_get_hz(clk_peri);
    printf("clk_peri frequency: %u MHz\n", clk_peri_freq/1000000);

    uint32_t clk_usb_freq = clock_get_hz(clk_usb);
    printf("clk_usb frequency: %u MHz\n", clk_usb_freq/1000000);

    uint32_t clk_adc_freq = clock_get_hz(clk_adc);
    printf("clk_adc frequency: %u MHz\n", clk_adc_freq/1000000);

    // Function to get the current VREG voltage in millivolts
     uint32_t reg_value = vreg_and_chip_reset_hw->vreg;
     // Shift the number right by 4 bits to bring bits 4-7 to positions 0-3 and then mask the lower 4 bits
    uint32_t vsel = (reg_value >> 4) & 0x0F;
    uint16_t vreg_mv = (vsel - 0b0110) * 50 + 850;  // According to datasheet
    printf("Current VREG: %d mV\n", vreg_mv);

    printf("clocks_hw->sleep_en0: 0x%X \n", clocks_hw->sleep_en0);
    printf("clocks_hw->sleep_en1: 0x%X \n", clocks_hw->sleep_en1);

    printf("clocks_hw->wake_en0: 0x%X \n", clocks_hw->wake_en0);
    printf("clocks_hw->wake_en1: 0x%X \n", clocks_hw->wake_en1);

    printf("\n");
    uart_tx_wait_blocking(UART);
}

int main()
{
    // GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(INT_PIN);
    gpio_set_dir(INT_PIN, GPIO_IN);
    gpio_pull_up(INT_PIN);

    last_interrupt_time = get_absolute_time();

    stdio_init_all();

    ledON();
    sleep_ms(500);
    ledOFF();
    
    printf("\n\n\nStartup!\n");
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
    } else {
        printf("Clean boot\n");
    }

    printf("Default frequencies the chip started up with:\n");
    showFreq();


    printf("Wait for 5 seconds to allow to measure current.\n");
    absolute_time_t start_time = get_absolute_time();
    while (absolute_time_diff_us(start_time, get_absolute_time()) < 5000000) {
        tight_loop_contents();  // Keep the CPU busy
    }

    // Busy wait current consumption at 3.3V: 25.0 mA

    // CLK_REF = XOSC
    // This is default like this, but just in case.
    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                    0, // No aux mux
                    XOSC_HZ,
                    XOSC_HZ);

    // CLK SYS = CLK_REF
    // This lowers the clk_sys from 125 MHz to 12 Mhz!
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0, // Using glitchless mux
                    XOSC_HZ,
                    XOSC_HZ);

    // This is the pll that was generating the 125 MHz for the clk_sys before.
    // Now that clk_sys runs from the XOSC, there is no need for it.
    pll_deinit(pll_sys);

    // CLK USB = 0MHz
    // This is a 48 MHz clock before shutting down.
    clock_stop(clk_usb);

    // CLK ADC = 0MHz
    // This is a 48 MHz clock before shutting down.
    clock_stop(clk_adc);

    // This is the pll that was generating the 48 MHz for clk_usb and clk_adc.
    // Now that they are both shut off, there is no need for it.
    pll_deinit(pll_usb);

    // CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable
    clock_configure(clk_peri,
                    0,  //not used for clk_peri!
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    XOSC_HZ,
                    XOSC_HZ);

    // The UART is going to be all confused with the new clocks, so we need to reinitialise.
    stdio_init_all();
    
    printf("The lowered frequencies just before going to sleep:\n");
    showFreq();


    // Save these register states to be able to restore afer waking up
    uint clock0_orig = clocks_hw->sleep_en0;
    uint clock1_orig = clocks_hw->sleep_en1;
    uint save = scb_hw->scr;

    printf("Going to sleep with dog and enabled interrupt...\n\n");
    watchdog_enable(WATCHDOG_TIMEOUT, 1);
    gpio_set_irq_enabled_with_callback(INT_PIN, GPIO_IRQ_EDGE_FALL, true, &my_isr);

    // Turn off all clocks when in sleep mode except for RTC - which is needed for the watchdog!
    clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;
    clocks_hw->sleep_en1 = 0x0;
    // Enable deep sleep at the proc
    scb_hw->scr = save | M0PLUS_SCR_SLEEPDEEP_BITS;



    __wfi();    // ----- THIS IS WHERE EXECUTION STOPS FOR THE DEEP SLEEP -----
    // Current consumption at 3.3V: 1.55 mA



    // If these registers are not restored, any __wfi() will take the RP2040 to deep sleep!
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;
    scb_hw->scr = save;
    
    // Initialize pll_sys for 125 MHz system clock
    pll_init(
        pll_sys,       // PLL instance
        1,             // REFDIV
        1500 * MHZ,    // VCO frequency
        6,             // POSTDIV1
        2              // POSTDIV2
    );

    // Configure clk_sys to use pll_sys
    clock_configure(
        clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        125 * MHZ,     // Input frequency from pll_sys
        125 * MHZ      // Output frequency (desired)
    );

    // Configure pll_usb to generate a 48 MHz clock
    pll_init(
        pll_usb,          // PLL instance
        1,                // Reference divider (REFDIV)
        480 * MHZ,        // VCO frequency (480 MHz)
        5,                // Post-divider 1 (POSTDIV1)
        2                 // Post-divider 2 (POSTDIV2)
    );

    // Configure the USB clock to use pll_usb
    clock_configure(
        clk_usb,          // Clock to configure
        CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, // Set source to pll_usb
        0,                // No additional muxing
        48 * MHZ,         // Input frequency (from pll_usb)
        48 * MHZ          // Output frequency (desired)
    );

    // Configure clk_adc to use pll_usb as its source and set it to 48 MHz
    clock_configure(
        clk_adc,                                            // Clock being configured
        CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,    // Use clk_usb as the source
        0,                                                  // No aux source for clk_adc
        48 * MHZ,                                           // Input frequency from pll_usb
        48 * MHZ                                            // Desired clk_adc frequency
    );

    // Restore clk_peri to original
    clock_configure(clk_peri,
                0,  //not used for clk_peri!
                CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                125 * MHZ,
                125 * MHZ
    );

    stdio_init_all();
    printf("Woke up from interrupt and restored frequencies:\n");
    showFreq();


    printf("Wait for 5 seconds to allow to measure current.\n");

    start_time = get_absolute_time();
    while (absolute_time_diff_us(start_time, get_absolute_time()) < 5000000) {
        tight_loop_contents();  // Keep the CPU busy
    }

    // Busy wait current consumption: 24.4 mA
    // What has not been restored?!

    printf("Demonstrating light sleep while waiting for more interrupts to pat the dog.\n");
    //uart_tx_wait_blocking(UART);
    while(1)
    {
        printf(".\n");
        sleep_ms(1000);
    }
    // Light sleep current consumption: 21.6 mA
} // main
