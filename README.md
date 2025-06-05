# Description
A novel relaxed queue leveraging the Window Decoupled structure by Adones Rukundo and the time-stamp-based algorithm introduced by Mike Dodds.

## System Requirements
  1. Linux-based operating system
  2. g++ compiler

## Experiment Methodology
### Running the Microbenchmark
Follow the steps below to run the benchmark interactively:
  1. Enter `s` to select a subject queue implementation.
  2. Enter `p` to configure parameters for the selected subject.
  3. Enter `e` to set the enqueue rate (percentage).
     The default value is **50%**.
  4. Enter `i` to start the microbenchmark.

### Running the Macrobenchmark
Follow the steps below to run the benchmark interactively:
  1. Enter `g` to generate a graph.
     This will create a binary file named `graph.bin`.
     If `graph.bin` has already been generated, you can enter `l` to load the graph.
  2. Enter `s` to select a subject queue implementation.
  3. Enter `p` to configure parameters for the selected subject.
  4. Enter `a` to start the macrobenchmark.
