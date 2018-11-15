#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <string>
#include <iostream>
#include "version.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    gui::MainWindow w;
    w.show();

    std::string logPath;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("log", po::value<std::string>(&logPath), "log file path")
            ("version", "print version")
            ;
        po::variables_map console_vm;
        po::store(po::parse_command_line(argc, argv, console_desc), console_vm);
        if (console_vm.count("help")) {
            std::cout << console_desc;
            return 0;
        }
        if (console_vm.count("version")) {
            std::cout << "logseer version " << g_version << std::endl;
            return 0;
        }
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    if (!logPath.empty()) {
        w.openLog(logPath);
    }

    return app.exec();
}
