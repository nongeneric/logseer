#include "ParsingTask.h"
#include "FileParser.h"

namespace seer {

    ParsingTask::ParsingTask(FileParser* fileParser) : _fileParser(fileParser) {}

    void ParsingTask::body() {
        _fileParser->index([&] (auto done, auto total) {
            reportProgress((done * 100) / total);
        });
    }

} // namespace seer
