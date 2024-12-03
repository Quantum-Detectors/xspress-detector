
#ifndef SRC_XSPRESSPLUGIN_H
#define SRC_XSPRESSPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "XspressDefinitions.h"

namespace FrameProcessor {

class XspressMemoryBlock
{
public:

  XspressMemoryBlock();
  virtual ~XspressMemoryBlock();
  void set_size(uint32_t frame_size, uint32_t max_frames);
  void reallocate();
  void reset();
  void add_frame(uint32_t frame_id, char *ptr);
  bool check_full();
  uint32_t frames();
  uint32_t size();
  uint32_t current_byte_size();
  char *get_data_ptr();

private:
  char *ptr_;
  uint32_t num_bytes_;
  uint32_t filled_size_;
  uint32_t frames_;
  uint32_t max_frames_;
  uint32_t frame_size_;

  /** Pointer to logger */
  LoggerPtr logger_;
};

    class XspressProcessPlugin : public FrameProcessorPlugin {
    public:
        XspressProcessPlugin();

        virtual ~XspressProcessPlugin();

        void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

        void requestConfiguration(OdinData::IpcMessage& reply);

        void configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);

        // version related functions
        int get_version_major();

        int get_version_minor();

        int get_version_patch();

        std::string get_version_short();

        std::string get_version_long();

    private:

        void set_number_of_channels(uint32_t num_channels);
        void set_number_of_aux(uint32_t num_aux);
        void setup_memory_allocation();
        
        // Plugin interface
        void process_frame(boost::shared_ptr <Frame> frame);

        void send_scalars(uint32_t last_frame_id, uint32_t num_scalars, uint32_t first_channel, uint32_t num_channels);

        uint32_t num_frames_;
        uint32_t num_energy_bins_;
        uint32_t num_aux_;
        uint32_t num_channels_;
        uint32_t frames_per_block_;
        uint32_t current_block_start_;
        uint32_t concurrent_processes_;
        uint32_t concurrent_rank_;
        uint32_t offset;
        std::string acq_id_;
        std::string live_view_name_;

        /** Time the last scalar message was sent */
        boost::posix_time::ptime last_scalar_send_time_;
        /** Memory block for scalars */
        void *scalar_memblock_;
        void *dtc_memblock_;
        void *inp_est_memblock_;
        void *sum_memblock_;
        /** Memory block for sum value */
        void calculate_sum(boost::shared_ptr<Frame> frame);
        /** Number of scalars recorded */
        uint32_t num_scalars_recorded_;


        std::vector<boost::shared_ptr<XspressMemoryBlock> > memory_ptrs_;

        /** Configuration constant for the acquisition ID used for meta data writing */
        static const std::string CONFIG_ACQ_ID;

        static const std::string CONFIG_PROCESS;
        static const std::string CONFIG_PROCESS_NUMBER;
        static const std::string CONFIG_PROCESS_RANK;

        static const std::string CONFIG_LIVE_VIEW_NAME;

        static const std::string CONFIG_FRAMES;

        static const std::string CONFIG_DTC_FLAGS;
        static const std::string CONFIG_DTC_PARAMS;
        
        static const std::string CONFIG_CHUNK;

        static const std::string CONFIG_OFFSET;

        /** Pointer to logger */
        LoggerPtr logger_;
    };

}


#endif //SRC_XSPRESSPLUGIN_H
