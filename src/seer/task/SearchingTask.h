#pragma once

#include "Task.h"
#include <string>
#include <memory>

namespace seer {

class FileParser;
class Index;
class Hist;

namespace task {

class SearchingTask : public Task {
    FileParser* _fileParser;
    std::string _text;
    bool _regex;
    bool _caseSensitive;
    bool _messageOnly;
    std::shared_ptr<Index> _index;
    std::shared_ptr<Hist> _hist;

public:
    SearchingTask(FileParser* fileParser,
                    Index* index,
                    std::string text,
                    bool regex,
                    bool caseSensitive,
                    bool messageOnly);
    std::shared_ptr<Index> index();
    std::shared_ptr<Hist> hist();

protected:
    void body() override;
};

} // namespace task
} // namespace seer
