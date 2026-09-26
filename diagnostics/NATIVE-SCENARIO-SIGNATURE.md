# Scenario graph signature boundary

`CScenarioFlowChartItemsList` is the game-used bitset for zone/clue membership.
IDs 31 and 63 used a signed `1 << bit` expression; the same bit position now
uses a 32-bit unsigned mask on Windows x64, Linux x86-64 and ARM64. The list
also refuses an out-of-range inner ID before indexing the signature vector.
Valid IDs keep the original word/bit mapping, including bit 31.

`PortableScenarioSignatureTests` checks IDs 0, 31, 32, 63 and 64 and the
absent neighboring bits. The original `scFlowChart.cpp` is compiled by the
Linux scenario library and Windows game build. This is a bitset arithmetic
regression, not a test that Lua scenario branching executes in a Linux
mission; that runtime gate remains open.
