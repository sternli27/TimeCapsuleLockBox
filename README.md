# TimeCapsuleLockBox
Here you can find all my project files to build your own DIY Time Capsule Box.

## Purpose
Such a box is perfect as a gift, which opens automatically after one or more years.
The integrated display counts down in days until the box opens on the pre programmed due date. Afterwards, the numbers count up again.
The display updates exactly 1x per day. This takes about 8s. The Teensy spends the rest of the day in deep sleep. Thus, a long battery life is guaranteed.

## Material
- Teensy 3.2
- Crystal 32.768 kHz, 12.5 pF to use the internal RTC on the Teensy 3.2
- Epaper Display Waveshare 2.9" 296 x 128 pixels
- Electromagnet Solenoid 12V 6W (German: Hubmagnet)
- Mosfet N-channel ex. IRLZ34N (low gate threshold voltage)
- Step Up Voltage Boost Converter (only used for solendoid) ex. LM2587
- Energizer Ultimate Lithium AA Batteries

## Electronic connection between the modules
| Teensy Pin  | Connected to                               |
| ----------- | ------------------------------------------ |
| Vin         | Battery Pack 1 positive                    |
| GND         | GND Battery Pack 1+2 + epaper GND (brown)  |
| 3.3V        | epaper VCC (grey)                          |
| VBat        | Battery Pack 2 positive (RTC)              |
| 7           | epaper BUSY (purple)                       |
| 8           | epaper RST (white)                         |
| 9           | epaper DC (green)                          |
| 10          | epaper CS (orange)                         |
| 11          | epaper DIN (blue)                          |
| 12          |                                            |
| 13          | epaper CLK (yellow)                        |
| 20          | Mosfet Gate                                |

The Mosfet at pin 20 switches the negative power supply to the StepUp converter.
So the negative supply for the StepUp Converter comes from the Mosfet Drain.
Mosfet source is connected to the GND of battery pack 3.
The positive supply for the StepUp Converter comes from the positive terminal on battery pack 3.
The solenoid is connected to the output of the StepUp Converter.

The solenoid is only actuated for a short time, but it should trip reliably within this short time. For this purpose, set the voltage at the boost converter somewhat higher (e.g. 18V), even if the solenoid is actually designed for 12V.

## Battery Setup
I have had very good experiences with the unfortunately somewhat expensive "Energizer Ultimate Lithium batteries". Don't use cheap batteries, or the project won't last to the end.

For this project I have divided the AA cells into three separate battery packs.
| Battery Pack  | Number of AA cells      | Consumer                              |
| ------------- | ----------------------- | ------------------------------------- |
| 1             | 3 cells in series       | Teensy 3.2 Vin 3.6-6.0V (used for daily updates) |
| 2             | 2 cells in series       | Teensy 3.2 RTC only                   |
| 3             | 4 or 6 cells in series  | Boost Converter powering the solenoid |

Caution: Remove the batteries from pack 1 during programming. The batteries have a different voltage than the USB supply.
Or see [here](https://www.pjrc.com/teensy/external_power.html) for other options.

If you need longer battery life, I recommend using 6 batteries with thein battery pack 1. So, 2 groups in parallel and within each group 3 batteries in series.

## CAD
I designed my box in Onshape. You can copy the project and customize it for your needs.
Check it out [here](https://cad.onshape.com/documents/37d5a27263dd0ff057f257a3/w/2627769e2c83fa2f752a5797/e/e0c9090dbd78b67fb374a487?renderMode=0&uiState=64a972349e21793d45eedffc)


