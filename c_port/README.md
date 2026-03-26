# C Port Workspace

## Folder layout
- src/: C source files
- include/: Header files
- bin/: Built executables
- out/csv/propagation: Propagation sweep CSV outputs
- out/csv/8psk: 8-PSK CSV outputs
- out/csv/64apsk: 64-APSK CSV outputs
- out/csv/256apsk: 256-APSK CSV outputs
- out/svg/propagation: Propagation SVG figures
- out/svg/8psk: 8-PSK SVG figures
- out/svg/64apsk: 64-APSK SVG figures
- out/svg/256apsk: 256-APSK SVG figures
- docs/: Notes on architecture alignment

## Main commands
- make all: Build everything
- make plot8: Run 8-PSK simulation and generate BER/constellation/diagnostic figures
- make plot64: Run 64-APSK simulation and generate BER/constellation/diagnostic figures
- make plot256: Run 256-APSK simulation and generate BER/constellation/diagnostic figures
- make plotapsk: Run both APSK simulations and generate figures
- make plotall: Run 8-PSK + APSK pipelines and generate all figures
- make clean: Remove bin/ and out/
- make plotall SEED=12345: Regenerate all figures with deterministic RNG seed
- make plotall AVG_WINDOWS=1: Single FFT snapshot (rougher spectrum)
- make plotall AVG_WINDOWS=16: Average 16 FFT windows (smoother spectrum)

## Reproducible runs
- By default, simulators use time-based seed.
- Set `SEED=<integer>` in Make targets to lock BER/constellation/spectrum outputs.
- Set `AVG_WINDOWS=<integer>` to control spectrum smoothing (default `1`).
- Example: `make clean && make plotall SEED=12345`

## Diagnostics generated per modulation
- time_domain_<tag>.csv and time_domain_upconversion_<tag>.svg
: time traces before and after upconversion.
- spectrum_before_upconversion_<tag>.csv and spectrum_before_upconversion_<tag>.svg
: FFT spectrum of the pre-upconversion path, shown on RF-centered GHz axis.
- spectrum_after_upconversion_<tag>.csv and spectrum_after_upconversion_<tag>.svg
: FFT spectrum after RF upconversion.
- rrc_response_<tag>.csv and rrc_response_<tag>.svg
: raised cosine filter frequency response.

## Simulink-like chain implemented
1. Symbol mapping
2. Upsampling (N=8)
3. Square-root raised cosine TX filter (R=0.2, span=16)
4. Optional RF upconversion (enabled)
5. AWGN channel
6. Optional RF downconversion (enabled)
7. Matched square-root raised cosine RX filter
8. Symbol timing pick, hard decision, BER calculation
9. FFT spectrum export and SVG plotting
