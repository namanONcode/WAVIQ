#include <QTest>
#include <QLabel>
#include <QLineEdit>
#include <QDockWidget>

#include "module1/QtVisualizationView.h"
#include "module1/VisualizationAnalysisExecutor.h"
#include "module1/VisualizationPresenter.h"
#include "module1/LoaderExceptions.h"

namespace { std::string testdata(const std::string& f) { return std::string(TESTDATA_DIR) + "/" + f; } }

class TestGui : public QObject {
    Q_OBJECT
private:
    static module1::VisualizationRequest all_products() {
        module1::VisualizationRequest r; r.region={0,1024}; r.waveform=r.constellation=r.spectrum=r.power_spectrum=r.spectrogram=true; return r;
    }
    static module1::PlotWidget* plot(module1::QtVisualizationView& v, const char* name) {
        return v.findChild<module1::PlotWidget*>(name);
    }
private slots:
    void testViewLifecycleAndLogsPlaceholder() {
        module1::QtVisualizationView view; module1::VisualizationPresenter presenter(&view); view.show_window(); QVERIFY(view.isVisible());
        QVERIFY(view.findChild<QDockWidget*>("logs_dock")); auto* label=view.findChild<QLabel*>("logs_placeholder"); QVERIFY(label); QVERIFY(label->text().contains("Logs will appear"));
    }
    void testRealHdf5TypedProductsReachPlots() {
        module1::QtVisualizationView view; module1::VisualizationPresenter presenter(&view);
        presenter.open_hdf5_dataset(testdata("real_subset.h5")); QCOMPARE(presenter.hdf5_frame_count(), size_t(20));
        presenter.load_hdf5_dataset_frame(0,1000000.0); presenter.request_visualizations(all_products());
        auto* waveform=plot(view,"waveform_plot"); auto* constellation=plot(view,"constellation_plot"); auto* spectrum=plot(view,"spectrum_plot"); auto* power=plot(view,"power_spectrum_plot"); auto* waterfall=plot(view,"waterfall_plot"); QVERIFY(waveform&&constellation&&spectrum&&power&&waterfall);
        QCOMPARE(waveform->rendered_point_count(),size_t(1024)); QCOMPARE(constellation->rendered_point_count(),size_t(1024));
        QCOMPARE(spectrum->rendered_point_count(),size_t(1024)); QCOMPARE(power->rendered_point_count(),size_t(1024));
        QCOMPARE(waterfall->rendered_row_count(),size_t(7)); QCOMPARE(waterfall->rendered_column_count(),size_t(256));
        QVERIFY(spectrum->x_axis_title().contains("Hz")); QVERIFY(power->y_axis_title().contains("relative"));
    }
    void testExplicitHdf5RateIsRequiredAndPropagated() {
        module1::QtVisualizationView view; module1::VisualizationPresenter presenter(&view); presenter.open_hdf5_dataset(testdata("real_subset.h5"));
        QVERIFY_EXCEPTION_THROWN(presenter.load_hdf5_dataset_frame(0,0.0),module1::MissingMetadataError);
        presenter.load_hdf5_dataset_frame(1,250000.0); QCOMPARE(presenter.current_signal().sample_rate(),250000.0);
    }
    void testAnalysisControlsUseTypedPathAndValidateRegion() {
        module1::QtVisualizationView view; module1::VisualizationPresenter presenter(&view); presenter.open_hdf5_dataset(testdata("real_subset.h5")); presenter.load_hdf5_dataset_frame(5,1000000.0);
        view.findChild<QLineEdit*>("analysis_region_start")->setText("100"); view.findChild<QLineEdit*>("analysis_region_length")->setText("64");
        QVERIFY(QMetaObject::invokeMethod(&view,"analyze")); auto* spectrum=plot(view,"spectrum_plot"); QVERIFY(spectrum); QCOMPARE(spectrum->rendered_point_count(),size_t(1024));
    }
    void testBackgroundDeliveryAndStaleRealRequests() {
        module1::BackgroundVisualizationAnalysisExecutor executor; module1::QtVisualizationView view; module1::VisualizationPresenter presenter(&view,&executor);
        presenter.open_hdf5_dataset(testdata("real_subset.h5")); presenter.load_hdf5_dataset_frame(0,1000000.0);
        auto old=all_products(); old.power_spectrum=false; old.spectrogram=false; presenter.request_visualizations(old);
        auto latest=all_products(); latest.waveform=latest.constellation=latest.spectrum=latest.spectrogram=false; presenter.request_visualizations(latest);
        executor.wait_for_all(); presenter.drain_visualization_completions(); auto* power=plot(view,"power_spectrum_plot"); auto* waterfall=plot(view,"waterfall_plot"); QVERIFY(power&&waterfall); QCOMPARE(power->rendered_point_count(),size_t(1024));
        QCOMPARE(waterfall->rendered_row_count(),size_t(0));
    }
};
QTEST_MAIN(TestGui)
#include "test_gui.moc"
