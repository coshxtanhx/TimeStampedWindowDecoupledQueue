# Time-Stamped Window Decoupled Queue
A novel relaxed queue leveraging the Window Decoupled structure by Adones Rukundo and the time-stamp-based algorithm introduced by Mike Dodds.

## System Requirements
  1. Linux-based operating system
  2. g++ compiler

## Experiment Methodology
### Running the Microbenchmark
Follow the steps below to run the microbenchmark interactively:
  1. Enter `s` to select a subject queue implementation.
  2. Enter `p` to configure parameters for the selected subject.
  3. Enter `e` to set the enqueue rate (percentage).
     The default value is 50%.
  4. Enter `m` to toggle the microbenchmark mode.
     - Throughput check (default)
     - Relaxation distance check
  5. Enter `i` to start the microbenchmark.

### Running the Macrobenchmark
Follow the steps below to run the macrobenchmark interactively:
  1. Enter `g` to generate a graph.
     This will create a binary file named `graph.bin`.
     If `graph.bin` has already been generated, you can enter `l` to load the graph.
  2. Enter `s` to select a subject queue implementation.
  3. Enter `p` to configure parameters for the selected subject.
  4. Enter `a` to start the macrobenchmark.

## Control Groups
  1. DQ LRU https://dl.acm.org/doi/abs/10.1145/2482767.2482789
  2. DQ TL-RR https://dl.acm.org/doi/abs/10.1145/2482767.2482789
  3. d-CBO https://dl.acm.org/doi/10.1145/3710848.3710892
  4. TS-interval https://doi.org/10.1145/2676726.2676963
  5. 2Dd https://arxiv.org/abs/1906.07105

## Miscellaneous
* All data structures are implemented in compliance with the C++ standard.
* The ABA problem and dereferencing dangling pointers was avoided by using epoch-based reclamation.
