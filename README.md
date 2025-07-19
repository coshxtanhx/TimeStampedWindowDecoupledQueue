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
  3. Enter `w` to set the width of the selected subject  
    (Only for 2Dd and d-CBO)
  3. Enter `e` to set the enqueue rate (percentage).  
     The default value is 50%.
  4. Enter `m` to toggle the microbenchmark mode.  
    - Throughput check (default)  
    - Relaxation distance check
  5. Enter `c` to toggle the scaling mode.  
    - Scaling with threads (default)  
    - Scaling with relaxation bound
  6. Enter `i` to start the microbenchmark.

### Running the Macrobenchmark
Follow the steps below to run the macrobenchmark interactively:
  1. Enter `g` to generate a graph. This will create a binary file name like `graph{}.bin`. If such a binary file has already been generated, you can enter `l` to load the graph.
     - Alpha (4.5M vertices, 53.6M edges / needs 222 MiB memory to generate)
     - Beta (4.5M vertices,	320M edges / needs 1.3 GiB memory to generate)
     - Gamma (12M vertices,	214M edges / needs 863 MiB memory to generate)
     - Delta (18M vertices,	1.28B edges / needs 4.9 GiB memory to generate)
     - Epsilon (21M vertices,	1.75B edges / needs 6.6 GiB memory to generate)
     - Zeta (25M vertices,	2.47B edges / needs 9.4 GiB memory to generate)
  2. Enter `s` to select a subject queue implementation.
  3. Enter `p` to configure parameters for the selected subject.
  4. Enter `w` to set the width of the selected subject  
    (Only for 2Dd and d-CBO)
  5. Enter `c` to toggle the scaling mode.  
    - Scaling with threads (default)  
    - Scaling with relaxation bound
  6. Enter `a` to start the macrobenchmark.

## Control Groups
  1. d-CBO https://dl.acm.org/doi/10.1145/3710848.3710892
  2. TS-interval, TS-CAS, TS-stutter, TS-atomic https://doi.org/10.1145/2676726.2676963
  3. 2Dd https://arxiv.org/abs/1906.07105

## Miscellaneous
* All data structures are implemented in compliance with the C++ standard.
* The ABA problem and dereferencing dangling pointers was avoided by using epoch-based reclamation.
* 128-bit CAS was not used.
