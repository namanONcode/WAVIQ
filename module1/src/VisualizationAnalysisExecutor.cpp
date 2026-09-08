#include "module1/VisualizationAnalysisExecutor.h"
#include "module1/VisualizationProcessors.h"

#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace module1 {
void InlineVisualizationAnalysisExecutor::submit(const ComplexSignal& signal, const VisualizationRequest& request,
                                                 const VisualizationAnalysisConfig& config, Completion completion) {
    try {
        completion(VisualizationProcessor::make_products(signal, request, config), nullptr);
    } catch (...) {
        completion({}, std::current_exception());
    }
}

struct BackgroundVisualizationAnalysisExecutor::State {
    struct PendingCompletion {
        Completion completion;
        VisualizationProducts products;
        std::exception_ptr error;
    };
    std::mutex mutex;
    std::vector<std::thread> workers;
    std::vector<PendingCompletion> completions;
};

BackgroundVisualizationAnalysisExecutor::BackgroundVisualizationAnalysisExecutor()
    : state_(std::make_unique<State>()) {}

BackgroundVisualizationAnalysisExecutor::~BackgroundVisualizationAnalysisExecutor() {
    wait_for_all();
}

void BackgroundVisualizationAnalysisExecutor::submit(const ComplexSignal& signal, const VisualizationRequest& request,
                                                     const VisualizationAnalysisConfig& config, Completion completion) {
    // Snapshotting preserves the caller's original model and gives the worker
    // stable data even if the presenter later loads another signal.
    ComplexSignal snapshot = signal;
    State* state = state_.get();
    std::lock_guard<std::mutex> lock(state->mutex);
    state->workers.emplace_back([state, signal = std::move(snapshot), request, config, completion = std::move(completion)]() mutable {
        VisualizationProducts products;
        std::exception_ptr error;
        try {
            products = VisualizationProcessor::make_products(signal, request, config);
        } catch (...) {
            error = std::current_exception();
        }
        std::lock_guard<std::mutex> completion_lock(state->mutex);
        state->completions.push_back({std::move(completion), std::move(products), error});
    });
}

void BackgroundVisualizationAnalysisExecutor::drain_completions() {
    std::vector<State::PendingCompletion> pending;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        pending.swap(state_->completions);
    }
    for (auto& completion : pending) completion.completion(std::move(completion.products), completion.error);
}

void BackgroundVisualizationAnalysisExecutor::wait_for_all() {
    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        workers.swap(state_->workers);
    }
    for (auto& worker : workers) if (worker.joinable()) worker.join();
}
} // namespace module1
