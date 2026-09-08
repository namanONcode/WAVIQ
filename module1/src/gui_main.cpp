#include <QApplication>
#include <QMetaObject>
#include "module1/QtVisualizationView.h"
#include "module1/VisualizationPresenter.h"
#include "module1/VisualizationAnalysisExecutor.h"
#include "module1/Logger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    module1::BackgroundVisualizationAnalysisExecutor executor;
    module1::QtVisualizationView view;
    
    module1::Logger::getInstance().setLogCallback([&view](const std::string& msg) {
        QMetaObject::invokeMethod(&view, [msg, &view]() {
            view.append_log(msg);
        }, Qt::QueuedConnection);
    });

    module1::VisualizationPresenter presenter(&view, &executor);
    
    view.show_window();
    
    return app.exec();
}
