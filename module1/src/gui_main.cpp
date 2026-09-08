#include <QApplication>
#include "module1/QtVisualizationView.h"
#include "module1/VisualizationPresenter.h"
#include "module1/VisualizationAnalysisExecutor.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    module1::BackgroundVisualizationAnalysisExecutor executor;
    module1::QtVisualizationView view;
    module1::VisualizationPresenter presenter(&view, &executor);
    
    view.show_window();
    
    return app.exec();
}
