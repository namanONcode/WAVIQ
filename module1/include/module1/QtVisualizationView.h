#pragma once

#include <QMainWindow>
#include <QPointF>
#include <QString>
#include <vector>

#include "module1/VisualizationPresenter.h"
#include "module1/VisualizationView.h"

class QCheckBox; class QComboBox; class QDockWidget; class QLabel; class QLineEdit;
class QPushButton; class QSpinBox; class QTimer; class QWidget; class QPainter; class QPaintEvent; class QWheelEvent; class QMouseEvent;

namespace module1 {
class PlotWidget : public QWidget {
    Q_OBJECT
public:
    enum class PlotType { TimeDomain, FFT, PowerSpectrum, Constellation, Waterfall };
    explicit PlotWidget(PlotType type, QWidget* parent = nullptr);
    void set_data(const std::vector<std::complex<float>>&); void set_fft_data(const std::vector<float>&);
    void set_waterfall_data(const std::vector<std::vector<float>>&);
    void set_waveform_data(const WaveformData&); void set_constellation_data(const ConstellationData&);
    void set_spectrum_data(const SpectrumData&); void set_power_spectrum_data(const PowerSpectrumData&);
    void set_spectrogram_data(const SpectrogramData&); void clear_data();
    size_t rendered_point_count() const; size_t rendered_row_count() const; size_t rendered_column_count() const;
    QString x_axis_title() const { return x_axis_title_; } QString y_axis_title() const { return y_axis_title_; }
protected:
    void paintEvent(QPaintEvent*) override; void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override; void mouseMoveEvent(QMouseEvent*) override;
private:
    PlotType type_; std::vector<WaveformPoint> waveform_; std::vector<ConstellationPoint> constellation_;
    std::vector<double> x_values_, y_values_; std::vector<float> values_; std::vector<std::vector<float>> matrix_;
    QString x_axis_title_, y_axis_title_; float zoom_factor_ = 1.0f; QPoint last_mouse_pos_; QPointF pan_offset_; QString cursor_readout_;
    void draw_axes(QPainter&) const;
};

class QtVisualizationView : public QMainWindow, public IVisualizationView {
    Q_OBJECT
public:
    explicit QtVisualizationView(QWidget* parent = nullptr); ~QtVisualizationView() override;
    void set_presenter(VisualizationPresenter*) override;
    void render_time_domain(const std::vector<std::complex<float>>&) override {} void render_fft(const std::vector<float>&) override {}
    void render_constellation(const std::vector<std::complex<float>>&) override {} void render_waterfall(const std::vector<std::vector<float>>&) override {}
    void render_waveform_data(const WaveformData&) override; void render_constellation_data(const ConstellationData&) override;
    void render_spectrum_data(const SpectrumData&) override; void render_power_spectrum_data(const PowerSpectrumData&) override;
    void render_spectrogram_data(const SpectrogramData&) override; void display_analysis_status(const std::string&, bool) override;
    void display_analysis_error(const std::string&) override; void display_metadata(const SignalMetadata&) override;
    void display_frame_info(const std::string&, int, int) override; void show_window() override;
private slots:
    void open_file(); void open_hdf5(); void on_frame_changed(int); void on_prev_frame(); void on_next_frame(); void analyze(); void drain_completions();
private:
    void setup_ui(); void setup_menus(); void prompt_manual_metadata(const std::string&); void load_hdf5_frame(int); void set_default_region(); bool valid_hdf5_rate(double&) const;
    VisualizationPresenter* presenter_ = nullptr; bool hdf5_open_ = false;
    PlotWidget *time_plot_ = nullptr, *const_plot_ = nullptr, *fft_plot_ = nullptr, *power_plot_ = nullptr, *waterfall_plot_ = nullptr;
    QLabel *metadata_label_ = nullptr, *frame_info_label_ = nullptr, *analysis_status_label_ = nullptr, *logs_placeholder_ = nullptr;
    QWidget* hdf5_nav_widget_ = nullptr; QSpinBox* frame_spinbox_ = nullptr;
    QPushButton *prev_btn_ = nullptr, *next_btn_ = nullptr, *analyze_btn_ = nullptr;
    QLineEdit *sample_rate_input_ = nullptr, *region_start_input_ = nullptr, *region_length_input_ = nullptr;
    QComboBox *axis_order_combo_ = nullptr, *fft_size_combo_ = nullptr;
    QCheckBox *waveform_check_ = nullptr, *constellation_check_ = nullptr, *spectrum_check_ = nullptr, *power_check_ = nullptr, *waterfall_check_ = nullptr;
    QTimer* completion_timer_ = nullptr; QDockWidget* logs_dock_ = nullptr;
};
} // namespace module1
