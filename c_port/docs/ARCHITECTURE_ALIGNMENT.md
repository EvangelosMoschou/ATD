# C vs Simulink Architecture Alignment

## Implemented signal chain (manual C)
1. Random symbol generation (uniform over constellation indices)
2. Symbol mapping (8-PSK / 64-APSK / 256-APSK constellations)
3. Upsampling by N=8 (zero insertion)
4. Square-root raised cosine TX FIR filter
   - roll-off R=0.2
   - span=16 symbols
   - taps = span*N + 1 = 129
5. AWGN channel (complex Gaussian noise)
6. Square-root raised cosine RX FIR matched filter
   - same taps as TX
7. Symbol timing pick at filter-compensated sample phase
   - total delay compensation = 2 * groupDelay
8. Hard-decision slicer (nearest constellation point)
9. BER computation over Eb/N0 sweep (0:2:20 dB)

## Constellation definitions
- 8-PSK: equally spaced unit-circle points, normalized to average symbol energy Es=1.
- 64-APSK: 8 rings x 8 points, radii [1, 1.791, 2.405, 2.980, 3.569, 4.235, 5.078, 6.536], ring phase offsets [pi/16, 3pi/16, ..., 15pi/16], then normalized to Es=1.
- 256-APSK: 8 rings x 32 points, same radii, offsets [pi/32, 3pi/32, ..., 15pi/32], then normalized to Es=1.

## Output artifacts
- BER CSV + BER SVG for each modulation.
- Constellation sample CSV + constellation SVG for each modulation.

## What remains different from full Simulink models
1. No RF upconversion/downconversion stage in this C version.
2. No spectrum analyzer emulation (FFT monitor blocks are replaced by offline SVG plots).
3. No explicit timing recovery loop or carrier recovery loop (fixed ideal timing phase is used).
4. No exact block-by-block parameter extraction from .slx (due unavailable toolboxes/coder).

## Practical implication
This C implementation is architecture-consistent with the complex-envelope chain and suitable for performance/portability migration, while remaining simpler than a full block-exact Simulink clone.