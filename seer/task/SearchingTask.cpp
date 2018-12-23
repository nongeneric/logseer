#include "SearchingTask.h"

#include "seer/Index.h"

namespace seer::task {

    SearchingTask::SearchingTask(FileParser* fileParser,
                                 Index* index,
                                 std::string text,
                                 bool caseSensitive)
        : _fileParser(fileParser),
          _text(text),
          _caseSensitive(caseSensitive),
          _index(new Index(*index)) {}

    std::shared_ptr<Index> SearchingTask::index() {
        return _index;
    }

    void SearchingTask::body() {
        _index->search(_fileParser, _text, _caseSensitive);
    }

} // namespace seer::task
