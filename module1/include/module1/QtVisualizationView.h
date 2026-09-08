#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QWidget>
#include <QPointF>

#include "module1/VisualizationView.h"
#include "module1/VisualizationPresenter.h"
#include "module1/HdfIqFrameDataset.h"
#include <memory>
#include <complex>
#include <vector>

namespace module1 {

// Simple custom widget for plotting using QPainter
class PlotWidget : public QWidget {
    Q_OBJECT
public:
    enum class PlotType { TimeDomain, FFT, Constellation, Waterfall };

    explicit PlotWidget(PlotType type, QWidget* parent = nullptr);
    void set_data(const std::vector<std::complex<float>>& samples);
    void set_fft_data(const std::vector<float>& spectrum);
    void set_waterfall_data(const std::vector<std::vector<float>>& spectrogram);
    void clear_data();

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    PlotType type_;
    std::vector<std::complex<float>> samples_;
    std::vector<float> spectrum_;
    std::vector<std::vector<float>> spectrogram_;
    
    // Zoom and pan
    float zoom_factor_ = 1.0f;
    QPoint last_mouse_pos_;
    QPointF pan_offset_;
    QString cursor_readout_;
};

class QtVisualizationView : public QMainWindow, public IVisualizationView {
    Q_OBJECT
public:
    explicit QtVisualizationView(QWidget* parent = nullptr);
    ~QtVisualizationView() override;

    // IVisualizationView overrides
    void set_presenter(VisualizationPresenter* presenter) override;
    void render_time_domain(const std::vector<std::complex<float>>& samples) override;
    void render_fft(const std::vector<float>& magnitude_spectrum) override;
    void render_constellation(const std::vector<std::complex<float>>& samples) override;
    void render_waterfall(const std::vector<std::vector<float>>& spectrogram) override;
    void display_metadata(const SignalMetadata& metadata) override;
    void display_frame_info(const std::string& modulation_name, int channel_condition, int snr_db) override;
    void show_window() override;

private slots:
    void open_file();
    void open_hdf5();
    void on_frame_changed(int frame);
    void on_prev_frame();
    void on_next_frame();

private:
    void setup_ui();
    void setup_menus();
    void prompt_manual_metadata(const std::string& filepath);
    void load_hdf5_frame(int frame);
    void load_audio_or_iq(const QString& path);

    VisualizationPresenter* presenter_ = nullptr;
    std::unique_ptr<HdfIqFrameDataset> current_hdf5_;

    // UI elements
    PlotWidget* time_plot_ = nullptr;
    PlotWidget* fft_plot_ = nullptr;
    PlotWidget* const_plot_ = nullptr;
    PlotWidget* waterfall_plot_ = nullptr;

    QLabel* metadata_label_ = nullptr;
    QLabel* frame_info_label_ = nullptr;

    // HDF5 navigation
    QWidget* hdf5_nav_widget_ = nullptr;
    QSpinBox* frame_spinbox_ = nullptr;
    QPushButton* prev_btn_ = nullptr;
    QPushButton* next_btn_ = nullptr;
    QLineEdit* sample_rate_input_ = nullptr;
    QComboBox* axis_order_combo_ = nullptr;
};

} // namespace module1
