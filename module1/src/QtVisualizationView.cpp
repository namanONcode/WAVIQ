#include "module1/QtVisualizationView.h"
#include "module1/LoaderExceptions.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace module1 {
namespace {
QString time_title(TimeAxisUnit unit) { return unit == TimeAxisUnit::Seconds ? "Time (s)" : "Sample index"; }
QString frequency_title(FrequencyAxisUnit unit) { return unit == FrequencyAxisUnit::Hertz ? "Frequency (Hz)" : "Frequency (cycles/sample)"; }
}

PlotWidget::PlotWidget(PlotType type, QWidget* parent) : QWidget(parent), type_(type) {
    setMinimumSize(300, 200); setMouseTracking(true);
}
void PlotWidget::set_data(const std::vector<std::complex<float>>& samples) { waveform_.clear(); for (size_t i=0;i<samples.size();++i) waveform_.push_back({i, static_cast<double>(i), samples[i].real(), samples[i].imag()}); x_axis_title_="Sample index"; y_axis_title_="Amplitude"; update(); }
void PlotWidget::set_fft_data(const std::vector<float>& spectrum) { values_=spectrum; x_values_.resize(spectrum.size()); for(size_t i=0;i<spectrum.size();++i)x_values_[i]=i; x_axis_title_="FFT bin"; y_axis_title_="Magnitude"; update(); }
void PlotWidget::set_waterfall_data(const std::vector<std::vector<float>>& value) { matrix_=value; x_axis_title_="Time"; y_axis_title_="Frequency"; update(); }
void PlotWidget::set_waveform_data(const WaveformData& value) { waveform_=value.points; x_axis_title_=time_title(value.x_unit); y_axis_title_="Amplitude"; update(); }
void PlotWidget::set_constellation_data(const ConstellationData& value) { constellation_=value.points; x_axis_title_="I amplitude"; y_axis_title_="Q amplitude"; update(); }
void PlotWidget::set_spectrum_data(const SpectrumData& value) { x_values_=value.frequency; values_=value.magnitude_db_relative; x_axis_title_=frequency_title(value.frequency_unit); y_axis_title_="Magnitude (dB, relative)"; update(); }
void PlotWidget::set_power_spectrum_data(const PowerSpectrumData& value) { x_values_=value.frequency; values_=value.power_db_relative; x_axis_title_=frequency_title(value.frequency_unit); y_axis_title_="Power (dB, relative)"; update(); }
void PlotWidget::set_spectrogram_data(const SpectrogramData& value) { x_values_=value.time; y_values_=value.frequency; matrix_=value.power_db_relative; x_axis_title_=time_title(value.time_unit); y_axis_title_=frequency_title(value.frequency_unit); update(); }
void PlotWidget::clear_data() { waveform_.clear(); constellation_.clear(); x_values_.clear(); y_values_.clear(); values_.clear(); matrix_.clear(); update(); }
size_t PlotWidget::rendered_point_count() const { return type_==PlotType::TimeDomain ? waveform_.size() : type_==PlotType::Constellation ? constellation_.size() : values_.size(); }
size_t PlotWidget::rendered_row_count() const { return matrix_.size(); }
size_t PlotWidget::rendered_column_count() const { return matrix_.empty()?0:matrix_.front().size(); }
void PlotWidget::wheelEvent(QWheelEvent* e) { zoom_factor_ = std::clamp(zoom_factor_ * (e->angleDelta().y()>0 ? 1.2f : 1.0f/1.2f), .2f, 20.f); update(); }
void PlotWidget::mousePressEvent(QMouseEvent* e) { if(e->button()==Qt::LeftButton) last_mouse_pos_=e->pos(); }
void PlotWidget::mouseMoveEvent(QMouseEvent* e) { if(e->buttons()&Qt::LeftButton){ QPoint d=e->pos()-last_mouse_pos_; pan_offset_+=d; last_mouse_pos_=e->pos(); } cursor_readout_=QString("X: %1, Y: %2").arg(e->pos().x()).arg(e->pos().y()); update(); }
void PlotWidget::draw_axes(QPainter& p) const { p.resetTransform(); p.setPen(Qt::lightGray); p.drawText(10,height()-8,x_axis_title_); p.save(); p.translate(12,height()/2); p.rotate(-90); p.drawText(0,0,y_axis_title_); p.restore(); p.drawText(10,20,cursor_readout_); }
void PlotWidget::paintEvent(QPaintEvent*) {
    QPainter p(this); p.fillRect(rect(),Qt::black); p.setRenderHint(QPainter::Antialiasing); p.translate(rect().center()+pan_offset_.toPoint()); p.scale(zoom_factor_,zoom_factor_);
    if(type_==PlotType::TimeDomain && waveform_.size()>1){ const float sx=std::max(1.f,200.f/static_cast<float>(waveform_.size())), mid=static_cast<float>(waveform_.size())/2.f; p.setPen(Qt::green); for(size_t i=1;i<waveform_.size();++i)p.drawLine((static_cast<float>(i)-1.f-mid)*sx,-waveform_[i-1].i*100,(static_cast<float>(i)-mid)*sx,-waveform_[i].i*100); p.setPen(Qt::red); for(size_t i=1;i<waveform_.size();++i)p.drawLine((static_cast<float>(i)-1.f-mid)*sx,-waveform_[i-1].q*100,(static_cast<float>(i)-mid)*sx,-waveform_[i].q*100); }
    else if(type_==PlotType::Constellation && !constellation_.empty()){ p.setPen(QColor(0,255,0,100)); for(const auto& v:constellation_)p.drawPoint(v.i*100,-v.q*100); p.setPen(Qt::gray); p.drawEllipse(QPointF(),100,100); }
    else if((type_==PlotType::FFT || type_==PlotType::PowerSpectrum) && values_.size()>1){ p.setPen(type_==PlotType::FFT?Qt::yellow:Qt::cyan); const float sx=std::max(1.f,200.f/static_cast<float>(values_.size())), mid=static_cast<float>(values_.size())/2.f; for(size_t i=1;i<values_.size();++i)p.drawLine((static_cast<float>(i)-1.f-mid)*sx,-values_[i-1]*2,(static_cast<float>(i)-mid)*sx,-values_[i]*2); }
    else if(type_==PlotType::Waterfall && !matrix_.empty()){ const float cw=std::max(.5f,300.f/static_cast<float>(matrix_.front().size())), ch=std::max(.5f,200.f/static_cast<float>(matrix_.size())), fm=static_cast<float>(matrix_.front().size())/2.f, tm=static_cast<float>(matrix_.size())/2.f; for(size_t t=0;t<matrix_.size();++t)for(size_t f=0;f<matrix_[t].size();++f){int c=std::clamp(static_cast<int>((matrix_[t][f]+120.f)/120.f*255.f),0,255);p.fillRect(QRectF((static_cast<float>(f)-fm)*cw,(static_cast<float>(t)-tm)*ch,cw,ch),QColor(c,255-c,0));}}
    else { p.setPen(Qt::white); p.drawText(-70,0,"No analysis data"); } draw_axes(p);
}

QtVisualizationView::QtVisualizationView(QWidget* parent):QMainWindow(parent){setup_ui();setup_menus();}
QtVisualizationView::~QtVisualizationView()=default;
void QtVisualizationView::set_presenter(VisualizationPresenter* p){presenter_=p;}
void QtVisualizationView::setup_ui(){
    auto* central=new QWidget(this); setCentralWidget(central); auto* main=new QVBoxLayout(central); auto* tabs=new QTabWidget(central); tabs->setObjectName("visualization_tabs");
    auto* tc=new QSplitter(Qt::Horizontal,tabs); time_plot_=new PlotWidget(PlotWidget::PlotType::TimeDomain,tc); time_plot_->setObjectName("waveform_plot"); const_plot_=new PlotWidget(PlotWidget::PlotType::Constellation,tc); const_plot_->setObjectName("constellation_plot"); tabs->addTab(tc,"Time & Constellation");
    auto* spectra=new QSplitter(Qt::Vertical,tabs); fft_plot_=new PlotWidget(PlotWidget::PlotType::FFT,spectra); fft_plot_->setObjectName("spectrum_plot"); power_plot_=new PlotWidget(PlotWidget::PlotType::PowerSpectrum,spectra); power_plot_->setObjectName("power_spectrum_plot"); tabs->addTab(spectra,"Spectra"); waterfall_plot_=new PlotWidget(PlotWidget::PlotType::Waterfall,tabs); waterfall_plot_->setObjectName("waterfall_plot"); tabs->addTab(waterfall_plot_,"Waterfall"); main->addWidget(tabs);
    metadata_label_=new QLabel("Metadata: None",central); metadata_label_->setObjectName("metadata_label"); frame_info_label_=new QLabel("Frame Info: None",central); frame_info_label_->setObjectName("frame_info_label"); main->addWidget(metadata_label_);main->addWidget(frame_info_label_);
    auto* analysis=new QWidget(central); auto* al=new QHBoxLayout(analysis); region_start_input_=new QLineEdit("0",analysis); region_start_input_->setObjectName("analysis_region_start"); region_length_input_=new QLineEdit(analysis); region_length_input_->setObjectName("analysis_region_length"); fft_size_combo_=new QComboBox(analysis); fft_size_combo_->setObjectName("fft_size_combo"); for(int n=64;n<=65536;n*=2)fft_size_combo_->addItem(QString::number(n)); fft_size_combo_->setCurrentText("1024"); waveform_check_=new QCheckBox("Waveform",analysis); constellation_check_=new QCheckBox("Constellation",analysis); spectrum_check_=new QCheckBox("Spectrum",analysis); power_check_=new QCheckBox("Power",analysis); waterfall_check_=new QCheckBox("Waterfall",analysis); for(auto* c:{waveform_check_,constellation_check_,spectrum_check_,power_check_,waterfall_check_})c->setChecked(true); analyze_btn_=new QPushButton("Analyze",analysis); analysis_status_label_=new QLabel("Ready",analysis); analysis_status_label_->setObjectName("analysis_status_label"); al->addWidget(new QLabel("Start:"));al->addWidget(region_start_input_);al->addWidget(new QLabel("Length:"));al->addWidget(region_length_input_);al->addWidget(new QLabel("FFT:"));al->addWidget(fft_size_combo_);for(auto*c:{waveform_check_,constellation_check_,spectrum_check_,power_check_,waterfall_check_})al->addWidget(c);al->addWidget(analyze_btn_);al->addWidget(analysis_status_label_);main->addWidget(analysis); connect(analyze_btn_,&QPushButton::clicked,this,&QtVisualizationView::analyze);
    hdf5_nav_widget_=new QWidget(central);auto* nav=new QHBoxLayout(hdf5_nav_widget_); sample_rate_input_=new QLineEdit(hdf5_nav_widget_);sample_rate_input_->setObjectName("hdf5_sample_rate");sample_rate_input_->setPlaceholderText("Required sample rate (Hz)");axis_order_combo_=new QComboBox(hdf5_nav_widget_);axis_order_combo_->addItems({"IThenQ","QThenI"});prev_btn_=new QPushButton("Previous",hdf5_nav_widget_);next_btn_=new QPushButton("Next",hdf5_nav_widget_);frame_spinbox_=new QSpinBox(hdf5_nav_widget_);frame_spinbox_->setMinimum(0);nav->addWidget(new QLabel("Sample Rate:"));nav->addWidget(sample_rate_input_);nav->addWidget(new QLabel("Axis (unverified):"));nav->addWidget(axis_order_combo_);nav->addWidget(prev_btn_);nav->addWidget(frame_spinbox_);nav->addWidget(next_btn_);main->addWidget(hdf5_nav_widget_);hdf5_nav_widget_->setVisible(false);connect(prev_btn_,&QPushButton::clicked,this,&QtVisualizationView::on_prev_frame);connect(next_btn_,&QPushButton::clicked,this,&QtVisualizationView::on_next_frame);connect(frame_spinbox_,QOverload<int>::of(&QSpinBox::valueChanged),this,&QtVisualizationView::on_frame_changed);
    completion_timer_=new QTimer(this);completion_timer_->setInterval(25);connect(completion_timer_,&QTimer::timeout,this,&QtVisualizationView::drain_completions);completion_timer_->start();resize(1200,700);
    logs_dock_=new QDockWidget("Logs",this);logs_dock_->setObjectName("logs_dock");logs_placeholder_=new QLabel("Logs will appear here when logging is available.",logs_dock_);logs_placeholder_->setObjectName("logs_placeholder");logs_placeholder_->setAlignment(Qt::AlignCenter);logs_dock_->setWidget(logs_placeholder_);addDockWidget(Qt::BottomDockWidgetArea,logs_dock_);
}
void QtVisualizationView::setup_menus(){auto* f=menuBar()->addMenu("File");auto* a=f->addAction("Open WAV/IQ...");connect(a,&QAction::triggered,this,&QtVisualizationView::open_file);a=f->addAction("Open HDF5 Dataset...");connect(a,&QAction::triggered,this,&QtVisualizationView::open_hdf5);auto* v=menuBar()->addMenu("View");v->addAction(logs_dock_->toggleViewAction());}
void QtVisualizationView::render_waveform_data(const WaveformData& d){time_plot_->set_waveform_data(d);} void QtVisualizationView::render_constellation_data(const ConstellationData& d){const_plot_->set_constellation_data(d);} void QtVisualizationView::render_spectrum_data(const SpectrumData& d){fft_plot_->set_spectrum_data(d);} void QtVisualizationView::render_power_spectrum_data(const PowerSpectrumData& d){power_plot_->set_power_spectrum_data(d);} void QtVisualizationView::render_spectrogram_data(const SpectrogramData& d){waterfall_plot_->set_spectrogram_data(d);}
void QtVisualizationView::display_analysis_status(const std::string& s,bool){analysis_status_label_->setText(QString::fromStdString(s));} void QtVisualizationView::display_analysis_error(const std::string& e){analysis_status_label_->setText("Analysis failed");QMessageBox::warning(this,"Analysis error",QString::fromStdString(e));}
void QtVisualizationView::display_metadata(const SignalMetadata& m){metadata_label_->setText(QString("Format: %1 | Type: %2 | Rate: %3 Hz | Source: %4 | Complex: %5").arg(QString::fromStdString(m.source_format)).arg(QString::fromStdString(m.sample_datatype)).arg(m.sample_rate_hz,0,'f',0).arg(QString::fromStdString(m.metadata_source)).arg(m.is_complex?"Yes":"No"));}
void QtVisualizationView::display_frame_info(const std::string& n,int c,int snr){frame_info_label_->setText(QString("Modulation: %1 | Channel: %2 | SNR: %3 dB").arg(QString::fromStdString(n)).arg(c==0?"Clean":"Multipath").arg(snr));}
void QtVisualizationView::show_window(){setWindowTitle("WAVIQ Visualizer");show();raise();}
void QtVisualizationView::open_file(){auto path=QFileDialog::getOpenFileName(this,"Open Signal File",{},"Audio (*.wav);;Raw IQ (*.iq)");if(path.isEmpty()||!presenter_)return;hdf5_open_=false;hdf5_nav_widget_->hide();try{presenter_->load_signal_file(path.toStdString());set_default_region();analyze();}catch(const MissingMetadataError&){prompt_manual_metadata(path.toStdString());}catch(const std::exception&e){QMessageBox::critical(this,"Error",e.what());}}
void QtVisualizationView::prompt_manual_metadata(const std::string& path){QDialog d(this);d.setWindowTitle("Manual Metadata Entry");auto*l=new QFormLayout(&d);auto*rate=new QLineEdit(&d);rate->setPlaceholderText("Required sample rate (Hz)");auto*type=new QComboBox(&d);type->addItems({"float32","int16","int8"});auto*endian=new QComboBox(&d);endian->addItems({"little","big"});l->addRow("Sample Rate (Hz):",rate);l->addRow("Data Type:",type);l->addRow("Byte Order:",endian);auto*buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);l->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;IqMetadataInput m;m.sample_rate_hz=rate->text().toDouble();m.sample_datatype=type->currentText().toStdString();m.byte_order=endian->currentText().toStdString();m.source="manual_entry";try{presenter_->load_iq_signal_with_metadata(path,m);set_default_region();analyze();}catch(const std::exception&e){QMessageBox::critical(this,"Error",e.what());}}
void QtVisualizationView::open_hdf5(){auto path=QFileDialog::getOpenFileName(this,"Open HDF5 Dataset",{},"HDF5 (*.h5)");if(path.isEmpty()||!presenter_)return;try{presenter_->open_hdf5_dataset(path.toStdString());hdf5_open_=true;hdf5_nav_widget_->show();sample_rate_input_->clear();frame_spinbox_->setMaximum(static_cast<int>(presenter_->hdf5_frame_count()-1));frame_spinbox_->setValue(0);analysis_status_label_->setText("Enter HDF5 sample rate to load a frame");}catch(const std::exception&e){QMessageBox::critical(this,"Error",e.what());}}
bool QtVisualizationView::valid_hdf5_rate(double& rate)const{bool ok=false;rate=sample_rate_input_->text().toDouble(&ok);return ok&&rate>0.;}
void QtVisualizationView::load_hdf5_frame(int frame){if(!hdf5_open_||!presenter_)return;double rate;if(!valid_hdf5_rate(rate)){analysis_status_label_->setText("A positive HDF5 sample rate is required");return;}try{presenter_->load_hdf5_dataset_frame(static_cast<size_t>(frame),rate,axis_order_combo_->currentIndex()==0?IqAxisOrder::IThenQ:IqAxisOrder::QThenI);set_default_region();analyze();}catch(const std::exception&e){QMessageBox::warning(this,"Error",e.what());}}
void QtVisualizationView::on_frame_changed(int f){load_hdf5_frame(f);}void QtVisualizationView::on_prev_frame(){frame_spinbox_->setValue(std::max(0,frame_spinbox_->value()-1));}void QtVisualizationView::on_next_frame(){frame_spinbox_->setValue(std::min(frame_spinbox_->maximum(),frame_spinbox_->value()+1));}
void QtVisualizationView::set_default_region(){if(!presenter_)return;const size_t n=presenter_->current_signal().sample_count();const size_t fft=fft_size_combo_->currentText().toULongLong();region_start_input_->setText("0");region_length_input_->setText(QString::number(std::min(n,fft)));}
void QtVisualizationView::analyze(){if(!presenter_||presenter_->current_signal().sample_count()==0)return;bool ok1=false,ok2=false;const auto start=region_start_input_->text().toULongLong(&ok1);const auto length=region_length_input_->text().toULongLong(&ok2);const auto fft=fft_size_combo_->currentText().toULongLong();if(!ok1||!ok2||length==0||start>presenter_->current_signal().sample_count()||length>presenter_->current_signal().sample_count()-start){display_analysis_error("Analysis region is outside the loaded signal");return;}if((spectrum_check_->isChecked()||power_check_->isChecked())&&length>fft){display_analysis_error("Spectrum analysis region length must not exceed FFT size");return;}VisualizationRequest r;r.region={static_cast<size_t>(start),static_cast<size_t>(length)};r.waveform=waveform_check_->isChecked();r.constellation=constellation_check_->isChecked();r.spectrum=spectrum_check_->isChecked();r.power_spectrum=power_check_->isChecked();r.spectrogram=waterfall_check_->isChecked();VisualizationAnalysisConfig c;c.fft_size=static_cast<size_t>(fft);presenter_->request_visualizations(r,c);}
void QtVisualizationView::drain_completions(){if(presenter_)presenter_->drain_visualization_completions();}
} // namespace module1
