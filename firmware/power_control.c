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

#include "pico/platform.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

const uint32_t GPIO_PULSE_INPUT = 15;

void gpios_callback( uint gpio, uint32_t events ) 
{
  gpio_put(LED_PIN, FLICKER_LED);
  busy_wait_us_32(1000);
  gpio_put(LED_PIN, 0);
}

int main()
{
  bi_decl(bi_program_description("Pico/555 Power Control Binary."));
  sleep_ms( 500 );

  gpio_init( GPIO_PULSE_INPUT );
  gpio_set_dir( GPIO_PULSE_INPUT, GPIO_IN );
  gpio_pull_up( GPIO_PULSE_INPUT );

  gpio_init( LED_PIN );
  gpio_set_dir( LED_PIN, GPIO_OUT );
  gpio_put(LED_PIN, 0);

  gpio_set_irq_enabled_with_callback( GPIO_PULSE_INPUT, GPIO_IRQ_EDGE_FALL, true, &gpios_callback );

  while(1);
}
