#pragma once

#include "Task.h"

namespace seer {

    class Index;
    class FileParser;
    class ILineParser;

    class IndexingTask : public Task {
        Index* _index;
        FileParser* _fileParser;
        ILineParser* _lineParser;

    public:
        IndexingTask(Index* index, FileParser* fileParser, ILineParser* lineParser);

    protected:
        void body() override;
    };

} // namespace seer
