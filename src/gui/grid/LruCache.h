#pragma once

#include <list>
#include <unordered_map>
#include <optional>

namespace gui {

template<typename T, typename Key>
struct Entry {
    typename std::list<Key>::iterator iter;
    T elem;
};

template<typename Key, typename T, typename KeyHash>
class LruCache {
    std::unordered_map<Key, Entry<T, Key>, KeyHash> _map;
    std::list<Key> _lru;
    int _capacity;

public:
    LruCache(size_t capacity) : _capacity(capacity) { }

    std::optional<T> lookup(Key key) {
        auto it = _map.find(key);
        if (it == end(_map))
            return {};
        _lru.erase(it->second.iter);
        _lru.push_front(key);
        it->second.iter = begin(_lru);
        return it->second.elem;
    }

    void insert(Key key, T elem) {
        auto it = _map.find(key);
        if (it == end(_map)) {
            _lru.push_front(key);
            _map[key] = { begin(_lru), elem };
        } else {
            _lru.erase(it->second.iter);
            _lru.push_front(key);
            it->second.iter = begin(_lru);
        }
        if ((int)_map.size() > _capacity) { // TODO: ssize
            auto lru = _lru.back();
            _lru.pop_back();
            _map.erase(_map.find(lru));
        }
    }

    bool erase(Key key) {
        auto it = _map.find(key);
        if (it != end(_map)) {
            _lru.erase(it->second.iter);
            _map.erase(it);
            return true;
        }
        return false;
    }

    void setCapacity(int capacity) {
        _capacity = capacity;
    }

    void clear() {
        _lru.clear();
        _map.clear();
    }
};

}
