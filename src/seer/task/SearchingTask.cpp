#include "SearchingTask.h"

#include "seer/Index.h"
#include "seer/Stopwatch.h"
#include "seer/Log.h"

#include <fmt/chrono.h>

namespace seer::task {

SearchingTask::SearchingTask(FileParser* fileParser,
                             Index* index,
                             std::string text,
                             bool regex,
                             bool caseSensitive,
                             bool unicodeAware,
                             bool messageOnly)
    : _fileParser(fileParser),
      _text(text),
      _regex(regex),
      _caseSensitive(caseSensitive),
      _unicodeAware(unicodeAware),
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
    log_info("search started");
    Stopwatch sw;

    auto result = _index->search(
        _fileParser,
        _text,
        _regex,
        _caseSensitive,
        _unicodeAware,
        _messageOnly,
        *_hist,
        [this] { return isStopRequested(); },
        [&](auto done, auto total) { reportProgress((done * 100) / total); });

    log_infof("search finished in {}", sw.msElapsed());

    if (!result)
        reportStopped();
}

} // namespace seer::task
