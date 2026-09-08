#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace features {

/**
 * @brief Higher-Order Cumulant (HOC) features for modulation classification.
 *
 * Cumulants are computed from the normalized, DC-removed, zero-mean signal.
 * The key cumulants for AMC (Automatic Modulation Classification) are:
 *
 *   C20 = E[x^2]           (2nd-order, useful for power measurement)
 *   C21 = E[|x|^2]         (always real and positive)
 *   C40 = E[x^4] - 3*(E[x^2])^2                  (4th-order)
 *   C41 = E[x^3 * x*] - 3*E[x^2]*E[|x|^2]        (4th-order, mixed)
 *   C42 = E[|x|^4] - |E[x^2]|^2 - 2*(E[|x|^2])^2 (4th-order, key discriminator)
 *   C60, C61, C62, C63     (6th-order, for finer discrimination)
 *
 * Notation: x* = complex conjugate; E[·] = sample average.
 *
 * Reference: Swami & Sadler, "Hierarchical Digital Modulation Classification
 * Using Cumulants", IEEE Trans. on Communications, 2000.
 */
struct CumulantValues {
    // 2nd-order cumulants
    float C20_mag;    ///< |C20| = |E[x^2]|
    float C21;        ///< C21 = E[|x|^2] (real, always positive)

    // 4th-order cumulants (normalized by C21^2 for scale invariance)
    float C40;        ///< Normalized C40
    float C41;        ///< Normalized |C41|
    float C42;        ///< Normalized C42

    // 6th-order cumulants (normalized by C21^3)
    float C60;        ///< Normalized C60
    float C61;        ///< Normalized |C61|
    float C62;        ///< Normalized C62
    float C63;        ///< Normalized |C63|
};

class CumulantFeatures {
public:
    /**
     * @brief Computes higher-order cumulants from IQ samples.
     *
     * The signal is first normalized to unit power: x_n = x / sqrt(E[|x|^2]).
     * Then cumulants are computed from the normalized signal and
     * reported as normalized (scale-invariant) values.
     *
     * @param data Input IQ signal. Must have at least 16 samples.
     * @return CumulantValues populated with 2nd, 4th, and 6th order cumulants.
     */
    static CumulantValues compute(const core::SignalData& data);

    /**
     * @brief Packs cumulants into the flat vector format used in SignalFeatures.
     *
     * Order: [C20_mag, C21, C40, C41, C42, C60, C61, C62, C63]
     *
     * @param cv The computed cumulant values.
     * @return Vector of 9 floats.
     */
    static std::vector<float> toVector(const CumulantValues& cv);
};

} // namespace features
} // namespace module2
