# nRF52_sample_code
Various snippets of sample code for Nordic nRF52 devices

# square_wave_comp
I needed to generate a 10kHz sine wave as an excitation signal for a sensor, and read the phase difference caused by the sensor value. More info here https://devzone.nordicsemi.com/f/nordic-q-a/89121/measuring-phase-shift-of-a-10khz-analog-signal-need-help-getting-started and https://www.eevblog.com/forum/beginners/generating-a-good-enough-10khz-sine-wave-from-a-square-one/

square_wave_comp is a sample program that generates a 10kHz square wave with a hardware timer and PPI, and uses the comparator to detect zero crossing. When detecting zero crossing, it uses PPI to capture the timer value in CC5. At any point in the code, CC5 contains the most recent value read.

The current code is using just one comparator in single ended mode, and instead of zero crossing detects when the AIN2 input rises above 2.4V. The square wave is on P1.08, and I connected an RC filter to that pin to generate a delayed signal for the AIN2 input, just as a test. In the final version of the prototype I will use an active SK lowpass filter to generate a sine wave from the square wave, read the timing of that signal compared to the hardware timer, and use that value as an offset for my sensor reading, then switch the comparator input to the sensor signal to detect phase shift. Periodically I will switch back the comparator to the sine wave excitation signal to minimize drift, and store any new value.

As you will see, I'm using 3 PPI events for the timer, even if 2 would be enough. I want to have the flexibility to define a phase shift between the timer start and the square wave rising edge, to account for the roughly 180 phase shift introduced by the active lowpass filter, to ensure that the zero crossing for the sensor signal happens inside the timer window. My original plan was to start another  crossing for the sensor signal happens inside the timer window. My original plan was to start another timer for the comparator, but I realized that nRF52840 timers have so many CC registers that only one timer was enough.
