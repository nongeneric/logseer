#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <string>
#include "Config.h"
#include "seer/CommandLineParser.h"
#include "seer/Log.h"
#include "seer/InstanceTracker.h"

int main(int argc, char *argv[]) {
    seer::CommandLineParser parser;
    if (parser.parse(argc, argv)) {
        seer::log_enable(parser.verbose());
        gui::g_Config.init();

        seer::InstanceTracker tracker(gui::g_socketName);

        if (!parser.help().empty()) {
            std::cout << parser.help() << std::endl;
            return 0;
        }

        if (!parser.version().empty()) {
            std::cout << parser.version() << std::endl;
            return 0;
        }

        if (tracker.connected()) {
            seer::log_infof("InstanceTracker connected to an existing instance");
            for (auto& path : parser.paths()) {
                seer::log_infof("Sending log path to active instance: %s", path);
                tracker.send(path);
            }
            return 0;
        }

        QApplication app(argc, argv);
        app.setStyle(QStyleFactory::create("Fusion"));
        gui::MainWindow w;
        w.show();

        w.setInstanceTracker(&tracker);

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
