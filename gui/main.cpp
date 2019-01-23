#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <string>
#include <iostream>
#include "Config.h"
#include "seer/CommandLineParser.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    gui::MainWindow w;
    w.show();

    seer::CommandLineParser parser;
    if (parser.parse(argc, argv)) {
        gui::g_Config.init();

        if (!parser.help().empty()) {
            std::cout << parser.help() << std::endl;
            return 0;
        }
        if (!parser.version().empty()) {
            std::cout << parser.version() << std::endl;
            return 0;
        }
        for (auto& path : parser.paths()) {
            w.openLog(path);
        }
    }

    return app.exec();
}
