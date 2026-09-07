#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace module1 {

// Ground-truth annotation for one frame of the HDF5 dataset. This is
// deliberately NOT folded into SignalMetadata: it's ML label data (what
// modulation/channel/SNR the dataset's authors captured), not signal
// interpretation metadata (sample rate/datatype/byte order) in the sense
// SignalMetadata represents. Mixing the two would special-case the core
// representation around one dataset, which the architecture explicitly
// avoids.
struct FrameLabels {
    int16_t modulation_id = 0;   // raw value from /y_mod; look up in ModulationLabelMap for the name
    int8_t channel_condition = 0; // raw value from /y_chan; see HdfIqFrameDataset::channel_meaning()
    int16_t snr_db = 0;           // raw value from /y_snr
};

using ModulationLabelMap = std::map<std::string, int>;

} // namespace module1
