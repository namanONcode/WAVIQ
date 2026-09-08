#pragma once

#include <functional>
#include <memory>
#include <exception>

#include "module1/SignalTypes.h"
#include "module1/VisualizationTypes.h"

namespace module1 {

// Scheduling infrastructure only. It has no loading, UI, or presentation
// policy. GUI integrations may provide a background implementation later.
class IVisualizationAnalysisExecutor {
public:
    // A failure is delivered on the owner's completion thread, never allowed
    // to escape a worker thread and terminate the desktop application.
    using Completion = std::function<void(VisualizationProducts, std::exception_ptr)>;
    virtual ~IVisualizationAnalysisExecutor() = default;
    virtual void submit(const ComplexSignal& signal, const VisualizationRequest& request,
                        const VisualizationAnalysisConfig& config, Completion completion) = 0;
    // Called by the Presenter's owning thread. Background implementations
    // invoke View-bound completions only here, never from worker threads.
    virtual void drain_completions() = 0;
};

// Deterministic synchronous executor for core applications and tests.
class InlineVisualizationAnalysisExecutor : public IVisualizationAnalysisExecutor {
public:
    void submit(const ComplexSignal& signal, const VisualizationRequest& request,
                const VisualizationAnalysisConfig& config, Completion completion) override;
    void drain_completions() override {}
};

class BackgroundVisualizationAnalysisExecutor : public IVisualizationAnalysisExecutor {
public:
    BackgroundVisualizationAnalysisExecutor();
    ~BackgroundVisualizationAnalysisExecutor() override;
    BackgroundVisualizationAnalysisExecutor(const BackgroundVisualizationAnalysisExecutor&) = delete;
    BackgroundVisualizationAnalysisExecutor& operator=(const BackgroundVisualizationAnalysisExecutor&) = delete;
    void submit(const ComplexSignal& signal, const VisualizationRequest& request,
                const VisualizationAnalysisConfig& config, Completion completion) override;
    void drain_completions() override;
    // Deterministic synchronization point for non-GUI integration tests and
    // orderly shutdown; delivery still occurs only through drain_completions.
    void wait_for_all();

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace module1
