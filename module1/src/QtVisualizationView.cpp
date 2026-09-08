#include "module1/QtVisualizationView.h"
#include "module1/LoaderFactory.h"
#include "module1/IqLoader.h"
#include "module1/LoaderExceptions.h"
#include "module1/IqMetadata.h"

#include <QPainter>
#include <QPaintEvent>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWheelEvent>
#include <QMouseEvent>

namespace module1 {

PlotWidget::PlotWidget(PlotType type, QWidget* parent) 
    : QWidget(parent), type_(type) {
    setMinimumSize(300, 200);
    setMouseTracking(true);
}

void PlotWidget::set_data(const std::vector<std::complex<float>>& samples) {
    samples_ = samples;
    update();
}

void PlotWidget::set_fft_data(const std::vector<float>& spectrum) {
    spectrum_ = spectrum;
    update();
}
void PlotWidget::set_waterfall_data(const std::vector<std::vector<float>>& spectrogram) {
    spectrogram_ = spectrogram;
    update();
}

void PlotWidget::clear_data() {
    samples_.clear();
    spectrum_.clear();
    spectrogram_.clear();
    update();
}

void PlotWidget::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) zoom_factor_ *= 1.2f;
    else zoom_factor_ /= 1.2f;
    update();
}

void PlotWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        last_mouse_pos_ = event->pos();
    }
}

void PlotWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - last_mouse_pos_;
        pan_offset_ += delta;
        last_mouse_pos_ = event->pos();
        update();
    }
    cursor_readout_ = QString("X: %1, Y: %2").arg(event->pos().x()).arg(event->pos().y());
    update();
}

void PlotWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::white);
    painter.drawText(10, 20, cursor_readout_);

    painter.translate(rect().center() + pan_offset_.toPoint());
    painter.scale(zoom_factor_, zoom_factor_);

    if (type_ == PlotType::TimeDomain && !samples_.empty()) {
        painter.setPen(Qt::green); // I
        for (size_t i = 1; i < samples_.size(); ++i) {
            painter.drawLine(i - 1 - samples_.size()/2, -samples_[i-1].real() * 100,
                             i - samples_.size()/2, -samples_[i].real() * 100);
        }
        painter.setPen(Qt::red); // Q
        for (size_t i = 1; i < samples_.size(); ++i) {
            painter.drawLine(i - 1 - samples_.size()/2, -samples_[i-1].imag() * 100,
                             i - samples_.size()/2, -samples_[i].imag() * 100);
        }
    } else if (type_ == PlotType::Constellation && !samples_.empty()) {
        painter.setPen(QColor(0, 255, 0, 100));
        for (const auto& s : samples_) {
            painter.drawPoint(s.real() * 100, -s.imag() * 100);
        }
        // Unit circle
        painter.setPen(Qt::gray);
        painter.drawEllipse(QPointF(0,0), 100, 100);
    } else if (type_ == PlotType::FFT) {
        if (spectrum_.empty()) {
            painter.setPen(Qt::white);
            painter.drawText(-50, 0, "No FFT Data (DSP not attached)");
        } else {
            painter.setPen(Qt::yellow);
            for (size_t i = 1; i < spectrum_.size(); ++i) {
                painter.drawLine(i - 1 - spectrum_.size()/2, -spectrum_[i-1] * 5,
                                 i - spectrum_.size()/2, -spectrum_[i] * 5);
            }
        }
    } else if (type_ == PlotType::Waterfall) {
        if (spectrogram_.empty()) {
            painter.setPen(Qt::white);
            painter.drawText(-50, 0, "No Waterfall Data (DSP not attached)");
        } else {
            // Basic waterfall drawing
            int time_steps = spectrogram_.size();
            if (time_steps > 0) {
                int freq_bins = spectrogram_[0].size();
                float cell_w = 1.0f;
                float cell_h = 1.0f;
                for (int t = 0; t < time_steps; ++t) {
                    for (int f = 0; f < freq_bins; ++f) {
                        int intensity = std::min(255, std::max(0, static_cast<int>(spectrogram_[t][f] * 255.0f)));
                        painter.fillRect(QRectF(f * cell_w - freq_bins/2.0f, t * cell_h - time_steps/2.0f, cell_w, cell_h), QColor(0, intensity, 0));
                    }
                }
            }
        }
    }
}

QtVisualizationView::QtVisualizationView(QWidget* parent) : QMainWindow(parent) {
    setup_ui();
    setup_menus();
}

QtVisualizationView::~QtVisualizationView() = default;

void QtVisualizationView::set_presenter(VisualizationPresenter* presenter) {
    presenter_ = presenter;
}

void QtVisualizationView::setup_ui() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* main_layout = new QVBoxLayout(central);

    // Plots
    QHBoxLayout* plots_layout = new QHBoxLayout();
    time_plot_ = new PlotWidget(PlotWidget::PlotType::TimeDomain);
    fft_plot_ = new PlotWidget(PlotWidget::PlotType::FFT);
    const_plot_ = new PlotWidget(PlotWidget::PlotType::Constellation);
    waterfall_plot_ = new PlotWidget(PlotWidget::PlotType::Waterfall);
    plots_layout->addWidget(time_plot_);
    plots_layout->addWidget(fft_plot_);
    plots_layout->addWidget(const_plot_);
    plots_layout->addWidget(waterfall_plot_);
    main_layout->addLayout(plots_layout);

    // Info Labels
    metadata_label_ = new QLabel("Metadata: None");
    metadata_label_->setObjectName("metadata_label");
    frame_info_label_ = new QLabel("Frame Info: None");
    frame_info_label_->setObjectName("frame_info_label");
    main_layout->addWidget(metadata_label_);
    main_layout->addWidget(frame_info_label_);

    // HDF5 Navigation
    hdf5_nav_widget_ = new QWidget();
    QHBoxLayout* nav_layout = new QHBoxLayout(hdf5_nav_widget_);
    
    prev_btn_ = new QPushButton("Previous");
    next_btn_ = new QPushButton("Next");
    frame_spinbox_ = new QSpinBox();
    frame_spinbox_->setMinimum(0);
    
    sample_rate_input_ = new QLineEdit("1000000"); // 1 MHz default
    sample_rate_input_->setPlaceholderText("Sample Rate (Hz)");
    
    axis_order_combo_ = new QComboBox();
    axis_order_combo_->addItem("IThenQ");
    axis_order_combo_->addItem("QThenI");
    
    nav_layout->addWidget(new QLabel("Sample Rate:"));
    nav_layout->addWidget(sample_rate_input_);
    nav_layout->addWidget(new QLabel("Axis:"));
    nav_layout->addWidget(axis_order_combo_);
    nav_layout->addWidget(prev_btn_);
    nav_layout->addWidget(frame_spinbox_);
    nav_layout->addWidget(next_btn_);
    
    main_layout->addWidget(hdf5_nav_widget_);
    hdf5_nav_widget_->setVisible(false);

    connect(prev_btn_, &QPushButton::clicked, this, &QtVisualizationView::on_prev_frame);
    connect(next_btn_, &QPushButton::clicked, this, &QtVisualizationView::on_next_frame);
    connect(frame_spinbox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &QtVisualizationView::on_frame_changed);
    connect(sample_rate_input_, &QLineEdit::textChanged, [this]() {
        if (current_hdf5_) load_hdf5_frame(frame_spinbox_->value());
    });
    connect(axis_order_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (current_hdf5_) load_hdf5_frame(frame_spinbox_->value());
    });

    resize(1200, 600);
}

void QtVisualizationView::setup_menus() {
    QMenu* file_menu = menuBar()->addMenu("File");
    
    QAction* open_file_action = new QAction("Open WAV/IQ...", this);
    connect(open_file_action, &QAction::triggered, this, &QtVisualizationView::open_file);
    file_menu->addAction(open_file_action);
    
    QAction* open_hdf5_action = new QAction("Open HDF5 Dataset...", this);
    connect(open_hdf5_action, &QAction::triggered, this, &QtVisualizationView::open_hdf5);
    file_menu->addAction(open_hdf5_action);
}

void QtVisualizationView::render_time_domain(const std::vector<std::complex<float>>& samples) {
    time_plot_->set_data(samples);
}

void QtVisualizationView::render_fft(const std::vector<float>& magnitude_spectrum) {
    fft_plot_->set_fft_data(magnitude_spectrum);
}

void QtVisualizationView::render_constellation(const std::vector<std::complex<float>>& samples) {
    const_plot_->set_data(samples);
}

void QtVisualizationView::render_waterfall(const std::vector<std::vector<float>>& spectrogram) {
    waterfall_plot_->set_waterfall_data(spectrogram);
}

void QtVisualizationView::display_metadata(const SignalMetadata& metadata) {
    QString info = QString("Format: %1 | Type: %2 | Rate: %3 Hz | Source: %4 | Complex: %5 | Bytes: %6 | Ch: %7")
        .arg(QString::fromStdString(metadata.source_format))
        .arg(QString::fromStdString(metadata.sample_datatype))
        .arg(metadata.sample_rate_hz, 0, 'f', 0)
        .arg(QString::fromStdString(metadata.metadata_source))
        .arg(metadata.is_complex ? "Yes" : "No")
        .arg(metadata.byte_order == ByteOrder::Little ? "Little" : "Big")
        .arg(metadata.channel_count);
    metadata_label_->setText(info);
}

void QtVisualizationView::display_frame_info(const std::string& modulation_name, int channel_condition, int snr_db) {
    QString info = QString("Modulation: %1 | Channel: %2 | SNR: %3 dB")
        .arg(QString::fromStdString(modulation_name))
        .arg(channel_condition == 0 ? "Clean" : "Multipath")
        .arg(snr_db);
    frame_info_label_->setText(info);
}

void QtVisualizationView::show_window() {
    this->setWindowTitle("WAVIQ Visualizer");
    this->show();
    this->raise();
}

void QtVisualizationView::open_file() {
    QString path = QFileDialog::getOpenFileName(this, "Open Signal File", "", "Audio (*.wav);;Raw IQ (*.iq)");
    if (path.isEmpty()) return;

    current_hdf5_.reset();
    hdf5_nav_widget_->setVisible(false);
    
    load_audio_or_iq(path);
}

void QtVisualizationView::load_audio_or_iq(const QString& path) {
    auto loader = LoaderFactory::create_loader(path.toStdString());
    if (!loader) {
        QMessageBox::critical(this, "Error", "Unsupported format");
        return;
    }

    try {
        ComplexSignal sig = loader->load(path.toStdString());
        if (presenter_) presenter_->on_signal_loaded(sig);
    } catch (const MissingMetadataError& e) {
        prompt_manual_metadata(path.toStdString());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void QtVisualizationView::prompt_manual_metadata(const std::string& filepath) {
    QDialog dialog(this);
    dialog.setWindowTitle("Manual Metadata Entry");
    QFormLayout* layout = new QFormLayout(&dialog);
    
    QLineEdit* rate_edit = new QLineEdit("1000000");
    QComboBox* type_combo = new QComboBox();
    type_combo->addItems({"float32", "int16", "int8"});
    QComboBox* endian_combo = new QComboBox();
    endian_combo->addItems({"little", "big"});
    
    layout->addRow("Sample Rate (Hz):", rate_edit);
    layout->addRow("Data Type:", type_combo);
    layout->addRow("Byte Order:", endian_combo);
    
    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(bbox);
    connect(bbox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        IqMetadataInput meta;
        meta.sample_rate_hz = rate_edit->text().toDouble();
        meta.sample_datatype = type_combo->currentText().toStdString();
        meta.byte_order = endian_combo->currentText().toStdString();
        meta.source = "manual_entry";
        
        IqLoader loader;
        try {
            ComplexSignal sig = loader.load_with_metadata(filepath, meta);
            if (presenter_) presenter_->on_signal_loaded(sig);
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }
}

void QtVisualizationView::open_hdf5() {
    QString path = QFileDialog::getOpenFileName(this, "Open HDF5 Dataset", "", "HDF5 (*.h5)");
    if (path.isEmpty()) return;

    try {
        current_hdf5_ = std::make_unique<HdfIqFrameDataset>(path.toStdString());
        hdf5_nav_widget_->setVisible(true);
        frame_spinbox_->setMaximum(current_hdf5_->frame_count() - 1);
        frame_spinbox_->setValue(0);
        load_hdf5_frame(0);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
        current_hdf5_.reset();
    }
}

void QtVisualizationView::load_hdf5_frame(int frame) {
    if (!current_hdf5_ || !presenter_) return;
    
    double rate = sample_rate_input_->text().toDouble();
    if (rate <= 0) rate = 1000000;
    
    IqAxisOrder order = axis_order_combo_->currentIndex() == 0 ? IqAxisOrder::IThenQ : IqAxisOrder::QThenI;
    
    try {
        ComplexSignal sig = current_hdf5_->load_frame(frame, rate, order);
        FrameLabels labels = current_hdf5_->labels_for_frame(frame);
        
        std::string mod_name = "Unknown";
        for (const auto& [name, id] : current_hdf5_->modulation_label_map()) {
            if (id == labels.modulation_id) {
                mod_name = name;
                break;
            }
        }
        
        presenter_->on_frame_loaded(sig, labels, mod_name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void QtVisualizationView::on_frame_changed(int frame) {
    load_hdf5_frame(frame);
}

void QtVisualizationView::on_prev_frame() {
    frame_spinbox_->setValue(std::max(0, frame_spinbox_->value() - 1));
}

void QtVisualizationView::on_next_frame() {
    frame_spinbox_->setValue(std::min(frame_spinbox_->maximum(), frame_spinbox_->value() + 1));
}

// --- Module 3 Overrides (Stubs for now) ---
void QtVisualizationView::render_decoded_bitstream(const std::vector<uint8_t>& payload) {
    // In a complete implementation, this would plot the hex/binary stream in a hex editor widget.
    qDebug() << "Module 3: Received payload of size:" << payload.size();
}

void QtVisualizationView::display_fec_metrics(float bit_error_rate, bool decode_success) {
    // This would update a status bar or metric panel.
    qDebug() << "Module 3: FEC Success:" << decode_success << "BER:" << bit_error_rate;
}

void QtVisualizationView::render_header_correlation(const std::vector<float>& correlation_metric) {
    // This would be plotted similarly to a time-domain wave to show correlation peaks.
    qDebug() << "Module 3: Received correlation metric of size:" << correlation_metric.size();
}

} // namespace module1
