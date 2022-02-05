#include "IndexingTask.h"

#include <gui/Config.h>
#include <seer/Index.h>

namespace seer::task {

IndexingTask::IndexingTask(Index* index,
                           FileParser* fileParser,
                           ILineParser* lineParser)
    : _index(index), _fileParser(fileParser), _lineParser(lineParser) {}

void IndexingTask::body() {
    auto finished = _index->index(
        _fileParser,
        _lineParser,
        gui::g_Config.generalConfig().maxThreads,
        [this] { return isStopRequested(); },
        [this](auto done, auto total) {
            reportProgress((done * 100) / total);
            waitPause();
        });
    if (!finished) {
        reportStopped();
    }
}

} // namespace seer::task
