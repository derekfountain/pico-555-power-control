/*
 * Pico 555 Power Control Firmware, a Raspberry Pi Pico power control test
 * Copyright (C) 2024 Derek Fountain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * cmake -DCMAKE_BUILD_TYPE=Debug ..
 * make -j10
 *
 * With the RPi Debug Probe:
 * 
 * sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" \
 *              -c "program power_control.elf verify reset exit"
 * sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
 *
 * gdb-multiarch power_control.elf
 *  target remote localhost:3333
 *  load
 *  monitor reset init
 *  continue
 *
 * With the home made Pico probe:
 *
 * sudo openocd -f interface/picoprobe.cfg -f target/rp2040.cfg \
 *              -c "program ./program power_control.elf verify reset exit"
 */

/*
 * Onboard LED shows life, flickering as the 555 pokes the Pico.
 * But it consumes power, so make it easy to turn off
 */
#define FLICKER_LED 1

#include <stdio.h>

#include "hardware/xosc.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/rosc.h"

#include "pico/sleep.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

const uint32_t GPIO_PULSE_INPUT   = 15;
const uint32_t GPIO_SCOPE_OUTPUT1 = 13;
const uint32_t GPIO_SCOPE_OUTPUT2 = 14;


void measure_freqs(void)
{
    uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n\n\n", f_clk_rtc);

    // Can't measure clk_ref / xosc as it is the ref
}

void gpios_callback( uint gpio, uint32_t events ) 
{
  /*
   * This handler is the first thing called wihen the Pico wakes up,
   * before, even, the main loop continues exectuing. So indicate
   * here (on the scope) that the Pico is awake.
   */
  gpio_put( GPIO_SCOPE_OUTPUT1, 1 );

  gpio_put(LED_PIN, FLICKER_LED);
  busy_wait_us_32(1000);
  gpio_put(LED_PIN, 0);
}

int main()
{
  bi_decl(bi_program_description("Pico/555 Power Control Binary."));
  stdio_init_all();
  sleep_ms( 500 );

  gpio_init( GPIO_PULSE_INPUT );
  gpio_set_dir( GPIO_PULSE_INPUT, GPIO_IN );
  gpio_pull_up( GPIO_PULSE_INPUT );

  gpio_init( GPIO_SCOPE_OUTPUT1 );
  gpio_set_dir( GPIO_SCOPE_OUTPUT1, GPIO_OUT );
  gpio_put( GPIO_SCOPE_OUTPUT1, 1 );

  gpio_init( GPIO_SCOPE_OUTPUT2 );
  gpio_set_dir( GPIO_SCOPE_OUTPUT2, GPIO_OUT );
  gpio_put( GPIO_SCOPE_OUTPUT2, 0 );

  gpio_init( LED_PIN );
  gpio_set_dir( LED_PIN, GPIO_OUT );
  gpio_put(LED_PIN, 0);

  gpio_set_irq_enabled_with_callback( GPIO_PULSE_INPUT, GPIO_IRQ_EDGE_FALL, true, &gpios_callback );

/*
 * Most examples I've seen for this type of code restore the clocks.
 * This post says it's not necessary:
 *   https://github.com/raspberrypi/pico-extras/issues/41#issuecomment-1368481410
 * So far he seems to be right, it works without restoration
 */
#ifdef RESTORE_CLOCKS
  /*
   * Store which clocks are enabled during sleep.
   */
  uint scb_orig = scb_hw->scr;
  uint clock0_orig = clocks_hw->sleep_en0;
  uint clock1_orig = clocks_hw->sleep_en1;
#endif

  while(1)
  {
//    printf("Hello world\n");
//    measure_freqs();

    gpio_set_dormant_irq_enabled( GPIO_PULSE_INPUT, GPIO_IRQ_EDGE_FALL, true );

    /* Scope output follows the Pico - low when Pico is dormant */
    gpio_put( GPIO_SCOPE_OUTPUT1, 0 );

    /*
     * This is a wrapper function in pico-extras, which are pulled into
     * this build. It configures the clocks and deinits the PLLs.
     */
    sleep_run_from_xosc();

    /*
     * There's a wrapper in pico-extras which is:
     * 
     *  sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high);
     *
     * which just does the gpio_set_dormant_irq_enabled() (which I
     * have above) and the gpio_acknowledge_irq() (which I have below),
     * surrounding an xosc_dormant() (which I have right here).
     *
     * I'm still not sure quite what happens here. The call to
     * sleep_run_from_xosc() calls rosc_disable(), but it doesn't
     * call xosc_disable(), which implies the XOSC isn't disabled.
     * The xosc_dormant() call below sends it dormant. So that
     * suggests the crystal oscillator is dormant but enabled during
     * the dormancy period? The call to clocks_init() below specifically
     * calls xosc_init(), but I'm not sure if that's redundant under
     * these circumstances.
     */
    xosc_dormant();

    gpio_acknowledge_irq( GPIO_PULSE_INPUT, GPIO_IRQ_EDGE_FALL );

    /*
     * The Ring Oscillator is disabled by the sleep_from_xosc() call.
     * This code hangs if it's not re-enabled. I don't know why. The
     * rosc must be needed for something I'm using here.
     */
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

#if RESTORE_CLOCKS
    /*
     * Restore the clocks which are enabled during sleep.
     */
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;
#endif

    /*
     * Reinitialise the clocks. This calls pll_init() for both clocks,
     * putting them back to their default state
     */
    clocks_init();

    /*
     * I saw instances while playing with this code where it wouldn't
     * come back up. Or at least, the LED stopped flickering which 
     * means the Pico isn't responding to the 555 poking it. Or at 
     * least might mean that. Putting a 1ms pause here stopped that
     * hanging. After a bit more experimentation I realised the problem
     * had gone away. I don't know why.
     *
     *  sleep_ms(1);
     *
     */

// Can't get USB to return, skipping for now
//    sleep_ms( 2000 );
//    stdio_init_all();
//    sleep_ms( 500 );
//    printf("Woken up\n");
//    measure_freqs();
  }
}
