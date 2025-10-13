Change log
==========

Changes made by Quantum Detectors to this repository are recorded below.


0.5.0+qd0.5
-----------

Changed:

- Increased the size of the X3X2ListModeFrameDecoder buffer to 12,800 TCP frames
  (100MB) per receiver as the default value of 5 meant that old buffer data was
  being overwritten before it was being processed when the Xspress system was
  generating a large number of events
- Added logging to check the time frames and time stamps do not decrease
  during an acquisition. This could happen if the processors fall behind the
  receivers and the receivers end up overwriting data that hasn't been
  processed in the circular buffer. This will be fixed in the future by changing
  the TCP receiver to drop packets that would otherwise overwrite the unprocessed
  data.
- Now sets the data sources in the Xspress channel control registers to ADC or
  playback based on the run flags setting to use the matching data source

Fixed:

- Now only send the last frame on flush_close_acquisition in list mode once -
  either from acquiring the desired number of time frames or from manually
  stopping the Odin data writers


0.5.0+qd0.4
-----------

Changed:

- Separated out X3X2 list mode datasets to 1 dataset per event field. The memory
  block has been split up into a base class and derived classes to manage the
  different datatypes used for each field. The datasets created are:

  - chX_time_frame (uint64)
  - chX_time_stamp (uint64)
  - chX_event_height (uint16)
  - chX_reset_flag (uint8)

- Changed how the X3X2 list mode plugin tracks acquisition completion
  as we receive multiple events with the end of frame marker set. Now
  use a map to track individual channel completion. The additional
  events are currently ignored.
- The X3X2 list mode processor plugin now sets the size of each memory
  block based on number of events stored (versus using bytes). The `frame_size`
  option under `xspress-list` in the JSON configuration file is used to define
  the number of events per memory block frame.

Fixed:

- Fixed issue with parsing the time frame bits in the X3X2 list mode plugin


0.5.0+qd0.3
-----------

Changed:

- Changed the number of processes used in list mode and MCA mode to be the same
  for support with X3X2 systems.
- Some tidy up of unused imports


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
