#ifndef SRC_X3X2LISTMODESCHEDULER_H
#define SRC_X3X2LISTMODESCHEDULER_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <stack>

#include "WorkQueue.h"
#include "X3X2ListModeProcessJob.h"
#include "X3X2ListModeFrameStore.h"

#define PROCESS_THREADS 8

namespace FrameProcessor
{

  /**
   * Generic class for managing a block of memory containing a single field
   * of list mode data.
   */
  class X3X2ListModeScheduler
  {
  public:
    X3X2ListModeScheduler();
    virtual ~X3X2ListModeScheduler();
    void set_number_of_time_frames(uint32_t time_frames);
    void set_channels(std::vector<uint32_t> channels, std::vector<uint32_t> marker_channels);
    void setup_frame_stores(uint32_t channel, const std::string& prefix, uint32_t frame_event_qty);
    void reset_time_stores();
    boost::shared_ptr<X3X2ListModeProcessJob> get_job();
    void release_job(boost::shared_ptr<X3X2ListModeProcessJob> job);
    std::vector<boost::shared_ptr<Frame> > process_frame(boost::shared_ptr<Frame> frame);
    void process_task();
    std::vector<boost::shared_ptr<Frame> > flush();
    void reset_acquisition();

  private:
    /** Pointer to logger */
    LoggerPtr logger_;

    /** Channels and offset */
    uint32_t channel_offset_;
    std::vector<uint32_t> channels_;
    std::vector<uint32_t> marker_channels_;

    /** Acquisition properties */
    uint32_t num_time_frames_;

    /** Pointer to worker queue thread */
    boost::thread *thread_[PROCESS_THREADS];

    /** Pointers to job queues for processing packets and results notification */
    boost::shared_ptr<WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> > > job_queue_;
    boost::shared_ptr<WorkQueue<boost::shared_ptr<X3X2ListModeProcessJob> > > res_queue_;

    /** Stack of processing job objects **/
    std::stack<boost::shared_ptr<X3X2ListModeProcessJob> > job_stack_;

    /** Timeframe Frame Store */
    // Memory blocks for event fields
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeFrameStoreTimeframe> > tf_store_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeFrameStoreTimestamp> > ts_store_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeFrameStoreEventHeight> > eh_store_ptrs_;
    std::map<uint32_t, boost::shared_ptr<X3X2ListModeFrameStoreResetFlag> > rf_store_ptrs_;

    // Timeframe and timestamp last value stores
    std::map<uint32_t, uint64_t> last_timeframe_store_;
    std::map<uint32_t, uint64_t> last_timestamp_store_;

    // Completed channels
    std::map<uint32_t, bool> completed_channels_;


    //    X3X2ListModeFrameStoreTimeframe tf_store_;
//    X3X2ListModeFrameStoreTimestamp ts_store_;
//    X3X2ListModeFrameStoreEventHeight eh_store_;
//    X3X2ListModeFrameStoreResetFlag rf_store_;
  };
}

#endif //SRC_X3X2LISTMODESCHEDULER_H
