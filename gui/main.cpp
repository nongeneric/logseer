#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <string>
#include <iostream>
#include "Config.h"
#include "seer/CommandLineParser.h"
#include "seer/Log.h"

int main(int argc, char *argv[]) {
    seer::CommandLineParser parser;
    if (parser.parse(argc, argv)) {
        seer::log_enable(parser.verbose());
        gui::g_Config.init();

        if (!parser.help().empty()) {
            std::cout << parser.help() << std::endl;
            return 0;
        }

        if (!parser.version().empty()) {
            std::cout << parser.version() << std::endl;
            return 0;
        }

        QApplication app(argc, argv);
        app.setStyle(QStyleFactory::create("Fusion"));
        gui::MainWindow w;
        w.show();

        for (auto& [path, parser] : gui::g_Config.sessionConfig().openedFiles) {
            w.openLog(path, parser);
        }

        for (auto& path : parser.paths()) {
            w.openLog(path);
        }

        return app.exec();
    }

    return 0;
}
