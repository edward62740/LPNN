# LPNN
This system runs an always-on neural network for detecting anomalies in audio. Specifically, this application involves the detection of glass breaking and forced entry sounds to provide security at points of ingress.

## Overview
The device consists of a NDP101 dedicated ML processor, SAMD21 microcontroller (on dev board), and an external host MCU for Internet connectivity. The neural network is detailed as below:
| Layer   | Dimension | Value     | Activation |
|---------|-----------|-----------|------------|
| Dense   | 256       | l1=.00001 | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 256       | l1=.0001  | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 256       | l1=.0001  | relu       |
| Dropout |           | 0.2       |            |
| Dense   | 2         |           | softmax    |
## Performance

Currently, the model runs (incl. microphone) at 517μA RMS.
