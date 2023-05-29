# LPNN
This system runs an always-on neural network for audio classification. Specifically, this project involves the detection of glass breaking and forced entry sounds to provide security at points of ingress. It runs at sub-mW power during inference, allowing for fully battery powered operation and thereby increased deployment flexibility.

## Overview
The device consists of a NDP101 dedicated AI accelerator, SAMD21 microcontroller (on dev board), and an external host MCU for Internet connectivity.

Note: This project is unfinished

## Design
### Hardware
Due to the lack of publicly available information regarding the NDP, direct integration onto a custom board was not feasible.<br>
Hence, its [development board](https://www.syntiant.com/tinyml) was used, and mounted onto a custom board. However, the dev board does not perform well as a low-power system, particularly the 3v3/0v9 LDOs which have high I<sub>Q</sub> of 115μA and 20μA respectively. As such, modifications were made to the board (i.e the regulators are bypassed and 3v3 is supplied externally). The LEDs and IMU were also desoldered.<br>
For some reason, the dev board does not have 3v3 routed to any of the pins (even though there are unused ones), and 0v9 lacks even a test pad.<br>
<img src="https://github.com/edward62740/LPNN/blob/master/Doc/pcb.jpeg" alt="PCB" width="35%" />

### Software (AI)
The audio signals are transformed into mel spectrograms, most of the feature extraction and DSP is done within the NDP101. Due to hardware limitations, only dense layers can be used, instead of convolutional layers which may be more suited to this case. The NDP101 is configured to interrupt the SAMD21 when a match is found and will communicate the details over SPI.<br>
The SAMD21 then passes the results over LPUART to the STM32WB SiP. Credits to [VOICe Dataset](https://dcase.community/challenge2017/task-rare-sound-event-detection-results) and [this project](https://docs.edgeimpulse.com/experts/prototype-and-concept-projects/vandalism-detection-audio-classification) for the audio datasets used.
The neural network architecture is detailed below:
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
<br>

![Spectrogram](https://github.com/edward62740/LPNN/blob/master/Doc/spec.png)
<br>*Spectrogram*

## Software (Connectivity)
The device communicates over a IPv6 Thread network and sends positive inference results to the cloud via this [bridge](https://github.com/edward62740/firebase-ingestor-rs).

## Performance

Currently, the model runs (incl. microphone) at 690 μA on 2v5 rail. The breakdown is below.
| Device               | Current Draw  (Avg) | Current Draw  (Worst) | Rail |
|----------------------|---------------------|-----------------------|------|
| NDP101               | 140μA               |                       |2v5/0v9|
| Microphone           |  ?                  |                       | 2v5  |
| SAMD21               | <10μA               |                       |      |
| 3v3 regulator (ext.) | 500nA               | 1000nA                | Vbat |
| 0v9 regulator        | 20μA                | 26μA                  | 2v5  |
| ~SPI NOR~              |                     |                       |      |
| Host MCU (ext.)      | <10μA               |                       | 2v5  |

It is possible to further improve this figure by decreasing the PDM clk, or by complete redesign of the board to allow for disconnecting the NOR flash etc from supply. 
Alternatively, a different microphone can be used, such as the MMICT5838 which consumes only 120μA at 768kHz. However, the power consumption is sufficient for the intended application, which yields about a year of battery life on 2xAA batteries.


The performance of the model used:
|                 | **Glass_break** | **Background** | **Uncertain** |
| --------------- | --------------- | -------------- | ------------- |
| **Glass_break** | 0.89            | 0.09           | 0.02          |
| **Background**  | 0.002           | 0.997          | 0.002         |
| **F1**          | 0.93            | 1.00           |               |

In practice, the model yields a higher FAR than ideal for a security application, where even very occasional false positives can prove to be disruptive to operations. For instance, the window duration is 968ms, so even a FPR of 1.65e-6 is one disruption a week. Additional window delay/suppression algorithms may need to be considered. However, this project is a sufficient proof-of-concept for intended further development. A future improvement will be to use the NDP120 (newer IC), which also provides 2d conv layers.

