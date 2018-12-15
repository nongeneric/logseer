#pragma once

#include "Task.h"

namespace seer {

    class FileParser;

    class ParsingTask : public Task {
        FileParser* _fileParser;

    public:
        ParsingTask(FileParser* fileParser);

    protected:
        void body() override;
    };

} // namespace seer
