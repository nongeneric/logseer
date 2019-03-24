#include "SearchingTask.h"

#include "seer/Index.h"

namespace seer::task {

    SearchingTask::SearchingTask(FileParser* fileParser,
                                 Index* index,
                                 std::string text,
                                 bool regex,
                                 bool caseSensitive)
        : _fileParser(fileParser),
          _text(text),
          _regex(regex),
          _caseSensitive(caseSensitive),
          _index(new Index(*index)) {}

    std::shared_ptr<Index> SearchingTask::index() {
        return _index;
    }

    std::shared_ptr<Hist> SearchingTask::hist() {
        return _hist;
    }

    void SearchingTask::body() {
        _hist = std::make_shared<Hist>(3000);
        _index->search(_fileParser, _text, _regex, _caseSensitive, *_hist);
    }

} // namespace seer::task
