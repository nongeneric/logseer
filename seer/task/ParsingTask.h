#pragma once

#include "Task.h"

namespace seer {

class FileParser;

namespace task {

class ParsingTask : public Task {
    FileParser* _fileParser;

public:
    ParsingTask(FileParser* fileParser);

protected:
    void body() override;
};

} // namespace task

} // namespace seer
