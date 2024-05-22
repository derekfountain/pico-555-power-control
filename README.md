# Experimentation with a Raspberry Pi Pico and a 555 timer.

I wanted to work out how to reduce the power consumption of
a Raspberry Pi Pico. After reading a bit online I decided the
only way to tell what works is to try some experimentation.

## Starting Point

Take a Raspberry Pi Pico and connect a 555 timer to one of its
GPIOs. Arrange for the 555 to poke the Pico about 10 times a
second. Treat that "poke" as an interrupt. On the interrupt,
light the onboard LED, busy-wait for 1ms, then turn the LED
off again. This workload is representative of nothing in
particular, but it allows me to see that the Pico is running.

I added a series resistor into the power supply so I could
measure the power comsumption of the Pico. The resistor is
nominally 1ohm, which is the smallest I had. The meter says
it's actually 1.3ohm, so I'm not sure what it really is. But
I'll always use the exact same resistor for these tests so
let's call it representitive only.

Running with a bench PSU at 5.0V into the Pico's VSYS.

### Starting Point Results

With the Pico running at its out of box configuration as per
the 'C' SDK, I see 23.9mV across the shunt resistor, which
indicates 18.38mA.

I turned the LED off by writing a 0 into the GPIO instead of
a 1, which leaves everything else the same. That gave me 23.7mV
across the resistor, so 18.23mA. Interesting start; the LED is
flickering away quite visibly in daylight, and consumes just
0.15mA. Bit of a digression, I don't care about the LED here,
but worth knowing.