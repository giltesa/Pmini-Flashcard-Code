# PM2040
An RP2040-based Flash cart for the Pokemon mini handheld.
<img src="./assets/image.png" alt="drawing" width="600"/>

## Hardware Side
### BOM
| **Reference** | **Value**| **Links**
|---------------|----------|----------|
| U3 | RP2040-Zero board ||
| R1, R2 | 100 kOhm resistor (0805) |[LCSC](https://www.lcsc.com/product-detail/Chip-Resistor-Surface-Mount_YAGEO-RC0805FR-07100KL_C96346.html)|

### PCB
The PCB can be ordered using the Gerber files. A width of **1.0 mm** should be chosen with ENIG surface.


## Soldering up the RP2040-Zero Board
We do not only make use of the RP2040-Zero board's castellated edges, but also the small contact pads on the back.
These pads are rather small, so make sure to align the board correctly, such that the VIAs of the PCB line up with the pads.
It can also be helpful to pre-tin the RP2040-Zero's additional pad with a **thin** layer before actually soldering the pads via the through-holes.

<img src="./assets/pcb.png" alt="drawing" width="600"/>
