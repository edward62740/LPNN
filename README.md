# LPNN
This system runs an always-on neural network for detecting anomalies in audio. Specifically, this application involves the detection of glass breaking and forced entry sounds to provide security at points of ingress.

## Overview
The device consists of a NDP101 dedicated ML processor, SAMD21 microcontroller (on dev board), and an external host MCU for Internet connectivity. The neural network is detailed as below:
| Layer   | Dimension | Value     | Activation |
|---------|-----------|-----------|------------|
| Input   | 1600,1    |           |            |
| Dense   | 256,1     | l1=.00001 | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 256,1     | l1=.00001 | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 256,1     | l1=.00001 | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 2,1       |           | softmax    |

## Design
Due to the lack of publicly available information regarding the NDP, direct integration onto a custom board was not reasonable within the timeframe of this project.<br>
Hence, its [development board](https://www.syntiant.com/tinyml) was used, and mounted onto a custom board. However, the dev board does not perform well as a low-power system, particularly the 3v3/0v9 LDOs which have high I<sub>Q</sub> of 115μA and 20μA respectively. As such, modifications were made to the board (i.e the regulators are bypassed and 3v3 is supplied externally). The LEDs and IMU were also desoldered.<br>
For some reason, the dev board does not have 3v3 routed to any of the pins (even though there are unused ones), and 0v9 lacks even a test pad.
## Performance

Currently, the model runs (incl. microphone) at 695 μA RMS. The breakdown is detailed below.
| Device               | Current Draw  (Typ) | Current Draw  (Worst) | Rail |
|----------------------|---------------------|-----------------------|------|
| NDP101               | 140μA               |                       |      |
| Microphone           | 620μA               | 700μA                 | 2v5  |
| SAMD21               |                     |                       |      |
| 3v3 regulator (ext.) | 500nA               | 1000nA                | Vbat |
| 0v9 regulator        | 20μA                | 26μA                  | 2v5  |
|                      |                     |                       |      |
|                      |                     |                       |      |

It is possible to further improve this figure by decreasing the PDM clk, or by complete redesign of the board to allow for disconnecting the NOR flash etc from supply. However, the current performance is sufficient for the intended application, which yields  years of battery life on 2xAA batteries.


