#ifndef SRC_X3X2LISTMODEPROCESSJOB_H
#define SRC_X3X2LISTMODEPROCESSJOB_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "gettime.h"

namespace FrameProcessor
{

  /**
   * Generic class for managing a block of memory containing a single field
   * of list mode data.
   */
  class X3X2ListModeProcessJob
  {
  public:
    X3X2ListModeProcessJob();
    virtual ~X3X2ListModeProcessJob();
    void init(uint32_t index, uint16_t *frame_data);
    uint32_t get_index();
    uint16_t get_channel();
    uint16_t *get_data_ptr();
    uint32_t get_event_qty();
    uint64_t *get_tf_ptr();
    uint64_t *get_ts_ptr();
    uint16_t *get_eh_ptr();
    uint8_t *get_rf_ptr();
    uint64_t get_first_timeframe();
    uint64_t get_last_timeframe();
    uint64_t get_first_timestamp();
    uint64_t get_last_timestamp();
    bool get_eof_marker();
    void process();

  private:
    uint32_t index_;
    uint16_t *data_;

    // Each job should hold pre-allocate enough memory for complete storage
    // of a packet full of events
    uint64_t *timeframe_;
    uint64_t *timestamp_;
    uint16_t *event_height_;
    uint8_t *reset_flag_;

    uint64_t *tf_ptr_;
    uint64_t *ts_ptr_;
    uint16_t *eh_ptr_;
    uint8_t *rf_ptr_;

    uint16_t channel_;
    uint32_t event_qty_;

    uint64_t first_time_frame_;
    uint64_t first_time_stamp_;
    uint64_t last_time_frame_;
    uint64_t last_time_stamp_;

    bool end_of_frame_marker_;

    /** Pointer to logger */
    LoggerPtr logger_;
  };
}

#endif //SRC_X3X2LISTMODEPROCESSJOB_H