#include <QTest>
#include <QApplication>
#include <QLabel>
#include <cmath>
#include <string>
#include <vector>

#include "module1/QtVisualizationView.h"
#include "module1/VisualizationPresenter.h"
#include "module1/HdfIqFrameDataset.h"
#include "module1/WavLoader.h"
#include "module1/IqLoader.h"
#include "module1/IqMetadata.h"
#include "module1/SignalTypes.h"

namespace {

std::string testdata(const std::string& filename) {
    return std::string(TESTDATA_DIR) + "/" + filename;
}

} // namespace

class TestGui : public QObject {
    Q_OBJECT

private slots:
    void testViewLifecycle() {
        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);
        view.show_window();
        QVERIFY(view.isVisible());
    }

    void testRealHdf5DatasetNavigationAndRendering() {
        module1::HdfIqFrameDataset ds(testdata("real_subset.h5"));
        QVERIFY(ds.frame_count() > 0);

        module1::ComplexSignal sig = ds.load_frame(0, 1000000.0);
        module1::FrameLabels labels = ds.labels_for_frame(0);

        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);

        presenter.on_frame_loaded(sig, labels, "QAM16");

        QVERIFY(presenter.current_signal().sample_count() == 1024);
        QVERIFY(presenter.has_labels());

        QLabel* meta_label = view.findChild<QLabel*>("metadata_label");
        QVERIFY(meta_label != nullptr);
        QVERIFY(meta_label->text().contains("hdf5"));
        QVERIFY(meta_label->text().contains("1000000 Hz"));

        QLabel* info_label = view.findChild<QLabel*>("frame_info_label");
        QVERIFY(info_label != nullptr);
        QVERIFY(info_label->text().contains("QAM16"));
        QVERIFY(info_label->text().contains(QString::number(labels.snr_db)));
    }

    void testRealWavSignalLoadingAndMetadata() {
        module1::WavLoader loader;
        module1::ComplexSignal sig = loader.load(testdata("real_subset.wav"));

        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);

        presenter.on_signal_loaded(sig);

        QVERIFY(sig.sample_count() > 0);

        QLabel* meta_label = view.findChild<QLabel*>("metadata_label");
        QVERIFY(meta_label != nullptr);
        QVERIFY(meta_label->text().contains("wav"));
        QVERIFY(meta_label->text().contains(QString::number(static_cast<int>(sig.metadata().sample_rate_hz)) + " Hz"));
    }

    void testRealIqSignalLoadingWithSigMF() {
        module1::IqLoader loader;
        module1::ComplexSignal sig = loader.load(testdata("real_subset_float32.iq"));

        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);

        presenter.on_signal_loaded(sig);

        QVERIFY(sig.sample_count() == 1024);

        QLabel* meta_label = view.findChild<QLabel*>("metadata_label");
        QVERIFY(meta_label != nullptr);
        QVERIFY(meta_label->text().contains("sigmf_sidecar"));
        QVERIFY(meta_label->text().contains("1000000 Hz"));
    }

    void testRealIqManualMetadataWorkflow() {
        module1::IqLoader loader;
        module1::IqMetadataInput meta;
        meta.sample_rate_hz = 250000.0;
        meta.sample_datatype = "float32";
        meta.source = "manual_entry";

        module1::ComplexSignal sig = loader.load_with_metadata(
            testdata("real_subset_float32_nosidecar.iq"), meta);

        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);

        presenter.on_signal_loaded(sig);

        QVERIFY(sig.sample_count() == 1024);

        QLabel* meta_label = view.findChild<QLabel*>("metadata_label");
        QVERIFY(meta_label != nullptr);
        QVERIFY(meta_label->text().contains("manual_entry"));
        QVERIFY(meta_label->text().contains("250000 Hz"));
    }

    void testRealWaterfallAndPlotRenderingFromDataset() {
        module1::HdfIqFrameDataset ds(testdata("real_subset.h5"));
        module1::ComplexSignal sig0 = ds.load_frame(0, 1000000.0);
        module1::ComplexSignal sig1 = ds.load_frame(1, 1000000.0);

        // Build a spectrogram matrix from real HDF5 frame sample magnitudes
        std::vector<std::vector<float>> spectrogram;
        
        auto compute_mags = [](const std::vector<std::complex<float>>& samples) {
            std::vector<float> mags;
            mags.reserve(samples.size());
            for (const auto& c : samples) {
                mags.push_back(std::abs(c));
            }
            return mags;
        };

        spectrogram.push_back(compute_mags(sig0.samples()));
        spectrogram.push_back(compute_mags(sig1.samples()));

        module1::QtVisualizationView view;
        module1::VisualizationPresenter presenter(&view);

        presenter.on_signal_loaded(sig0);

        view.render_time_domain(sig0.samples());
        view.render_constellation(sig0.samples());
        view.render_waterfall(spectrogram);

        QVERIFY(presenter.current_signal().sample_count() == 1024);
        QVERIFY(spectrogram.size() == 2);
        QVERIFY(spectrogram[0].size() == 1024);
    }
};

QTEST_MAIN(TestGui)
#include "test_gui.moc"
