# ThermoLang FPGA Contract v0

## Numeric format
- DATA_WIDTH = 16
- FRAC_BITS = 12
- Signed fixed-point format = Q4.12

## Spin encoding
- spin = +1 encoded as 2'sd1
- spin = -1 encoded as -2'sd1
- internal port width for spins = signed [1:0]

## Temperature
- temperature is a 16-bit fixed-point scalar consumed by each SPU cell
- temperature = 0 means frozen / greedy limit

## Coupling and bias config
Per cell, config words are ordered as:
1. h
2. j_north
3. j_south
4. j_east
5. j_west

## Grid policy
- hardware target is a 4x4 toroidal nearest-neighbor lattice
- north/south/east/west wrap around at edges

## Update policy
- simulation twin uses 2-phase checkerboard updates
- phase 0 updates cells with `(row + col) % 2 == 0`
- phase 1 updates cells with `(row + col) % 2 == 1`

## Reset polarity
- top-level SPU path uses active-low rst_n
- standalone TRNG path may use active-high rst

## TRNG interface
- TRNG emits rand_out[15:0]
- SPU consumes internal TRNG output in hardware
- simulation uses deterministic per-cell LFSR seeds under `SIMULATION`

## Schedule file format
Plain text:
- initial_temp <float>
- cooling_rate <float>
- steps <int>

## Output paths
Compiler/simulation artifacts should be stored under:
- `out/fpga/<example>/config.mem`
- `out/fpga/<example>/schedule.txt`
- `out/fpga/<example>/run.log`

## UART protocol version
- reserved for host-controlled board path
- version = TLNG_FPGA_ABI_V1