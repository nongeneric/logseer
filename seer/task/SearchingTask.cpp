#include "SearchingTask.h"

#include "seer/Index.h"

namespace seer::task {

SearchingTask::SearchingTask(FileParser* fileParser,
                             Index* index,
                             std::string text,
                             bool regex,
                             bool caseSensitive,
                             bool messageOnly)
    : _fileParser(fileParser),
      _text(text),
      _regex(regex),
      _caseSensitive(caseSensitive),
      _messageOnly(messageOnly),
      _index(std::make_shared<Index>(*index)) {}

std::shared_ptr<Index> SearchingTask::index() {
    return _index;
}

std::shared_ptr<Hist> SearchingTask::hist() {
    return _hist;
}

void SearchingTask::body() {
    _hist = std::make_shared<Hist>(3000);
    _index->search(
        _fileParser,
        _text,
        _regex,
        _caseSensitive,
        _messageOnly,
        *_hist,
        [this] { return isStopRequested(); },
        [&](auto done, auto total) { reportProgress((done * 100) / total); });
}

} // namespace seer::task
