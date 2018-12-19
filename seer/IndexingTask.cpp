#include "IndexingTask.h"

#include "Index.h"

namespace seer {

    IndexingTask::IndexingTask(Index* index,
                               FileParser* fileParser,
                               ILineParser* lineParser)
        : _index(index), _fileParser(fileParser), _lineParser(lineParser) {}

    void IndexingTask::body() {
        _index->index(_fileParser, _lineParser, [=] (auto done, auto total) {
            reportProgress((done * 100) / total);
            waitPause();
        });
    }
} // namespace seer
