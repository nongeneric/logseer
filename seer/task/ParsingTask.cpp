#include "ParsingTask.h"
#include "seer/FileParser.h"

namespace seer::task {

    ParsingTask::ParsingTask(FileParser* fileParser) : _fileParser(fileParser) {}

    void ParsingTask::body() {
        _fileParser->index([&] (auto done, auto total) {
            reportProgress((done * 100) / total);
        }, [=] { return this->isStopRequested(); });
    }

} // namespace seer
