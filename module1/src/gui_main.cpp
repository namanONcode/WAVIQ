#include <QApplication>
#include "module1/QtVisualizationView.h"
#include "module1/VisualizationPresenter.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    module1::QtVisualizationView view;
    module1::VisualizationPresenter presenter(&view);
    
    view.show_window();
    
    return app.exec();
}
