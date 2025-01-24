Change log
==========

Changes made by Quantum Detectors to this repository are recorded below.

0.5.0+qd0.1
-----------

Added:

- WIP of X3X2 list mode decoder plugin

Changed:

- C++ control server
  - now configures X3X2 clocks
  - now enables resets in list mode
  - Xspress library configure call explicitly requests list readout mode
- Python control server
  - requests X3X2 list mode decoder plugin on configure to list mode
  - changed number of channels used in list mode
  - changed frame receiver IP/port configuration (currently listens to
  scalars socket to leave event TCP/IP socket free for Python)

Fixed:

- Now use Xspress library method `xsp3_histogram_read4d` when reading X3X2 MCA
  data
- Set num_cards when detector is configured in control server
