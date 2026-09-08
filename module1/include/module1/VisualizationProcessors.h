#pragma once

#include "module1/SignalTypes.h"
#include "module1/VisualizationTypes.h"

namespace module1 {

// Qt-free processors. All output is derived; ComplexSignal is read only.
class WaveformProcessor {
public:
    static WaveformData make_data(const ComplexSignal& signal, AnalysisRegion region, size_t point_budget);
};

class ConstellationProcessor {
public:
    static ConstellationData make_data(const ComplexSignal& signal, AnalysisRegion region, size_t point_budget);
};

class SpectrumProcessor {
public:
    static SpectrumData make_magnitude_data(const ComplexSignal& signal, AnalysisRegion region,
                                             const VisualizationAnalysisConfig& config);
    static PowerSpectrumData make_power_data(const ComplexSignal& signal, AnalysisRegion region,
                                             const VisualizationAnalysisConfig& config);
};

class SpectrogramProcessor {
public:
    static SpectrogramData make_data(const ComplexSignal& signal, AnalysisRegion region,
                                     const VisualizationAnalysisConfig& config);
};

class VisualizationProcessor {
public:
    static VisualizationProducts make_products(const ComplexSignal& signal,
                                               const VisualizationRequest& request,
                                               const VisualizationAnalysisConfig& config);
};

} // namespace module1
