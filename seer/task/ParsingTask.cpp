#include "ParsingTask.h"
#include "seer/FileParser.h"
#include <assert.h>

namespace seer::task {

ParsingTask::ParsingTask(FileParser* fileParser) : _fileParser(fileParser) {
    assert(fileParser);
}

void ParsingTask::body() {
    _fileParser->index([&] (auto done, auto total) {
        reportProgress((done * 100) / total);
    }, [this] { return this->isStopRequested(); });
}

} // namespace seer
