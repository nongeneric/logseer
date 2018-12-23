#pragma once

#include "Task.h"
#include <string>
#include <memory>

namespace seer {

    class FileParser;
    class Index;

    namespace task {

        class SearchingTask : public Task {
            FileParser* _fileParser;
            std::string _text;
            bool _caseSensitive;
            std::shared_ptr<Index> _index;

        public:
            SearchingTask(FileParser* fileParser,
                          Index* index,
                          std::string text,
                          bool caseSensitive);
            std::shared_ptr<Index> index();

        protected:
            void body() override;
        };

    } // namespace task
} // namespace seer
