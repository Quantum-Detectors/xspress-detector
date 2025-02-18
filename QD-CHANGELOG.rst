Change log
==========

Changes made by Quantum Detectors to this repository are recorded below.

0.5.0+qd0.2
-----------

Added:

- Initial version of X3X2 processor plugin. This produces 1D frames containing
  three elements per event as time_frame, time_stamp and event_height.

Fixed:

- Detector status now goes idle when stopping acquisition in X3X2 list mode

Changed:

- Enabled X3X2 list mode receiver plugins
- The Xspress FP adapter now hard-codes the number of HDF5 frames to 0
  and sends the number of frames to the processors. These are configured
  with the number of time frames to collect and will stop the acquisition.
  Zero frames to collect will mean continuous processing.


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
