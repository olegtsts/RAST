#include <unordered_map>
#include <set>

#include "types.h"

template <typename TKey, typename TValue, typename THash = std::hash<TKey>>
class RankedMap {
public:
    using iterator=typename std::unordered_map<TKey, TValue, THash>::iterator;
    using const_iterator=typename std::unordered_map<TKey, TValue, THash>::const_iterator;

    iterator Find(const TKey& key) noexcept {
        return key_value.find(key);
    }

    const_iterator Find(const TKey& key) const noexcept {
        return key_value.find(key);
    }

    iterator begin() noexcept {
        return key_value.begin();
    }

    const_iterator begin() const noexcept {
        return key_value.begin();
    }

    iterator end() noexcept {
        return key_value.end();
    }

    const_iterator end() const noexcept {
        return key_value.end();
    }

    iterator GetLowest() noexcept {
        if (value_key.size() == 0) {
            return key_value.end();
        } else {
            return Find(value_key.begin()->second);
        }
    }

    const_iterator GetLowest() const noexcept {
        if (value_key.size() == 0) {
            return key_value.end();
        } else {
            return Find(value_key.begin()->second);
        }
    }

    void Erase(const TKey& key) {
        typename std::unordered_map<TKey, TValue, THash>::iterator it = key_value.find(key);
        if (it == key_value.end()) {
            return;
        }
        typename std::pair<TValue, TKey> pair = std::make_pair(it->second, key);
        key_value.erase(it);
        value_key.erase(pair);
    }

    void Update(const TKey& key, const TValue& value) {
        typename std::unordered_map<TKey, TValue, THash>::iterator it = key_value.find(key);
        typename std::pair<TValue, TKey> old_pair;
        bool has_key = it != key_value.end();
        typename std::pair<TValue, TKey> pair = std::make_pair(value, key);
        if (has_key) {
            old_pair = std::make_pair(it->second, it->first);
            if (old_pair == pair) {
                return;
            }
        }
        try {
            value_key.insert(pair);
            key_value[key] = value;
        } catch(...) {
            value_key.erase(pair);
            throw;
        }
        if (has_key) {
            value_key.erase(old_pair);
        }
    }

    size_t GetSize() const {
        return key_value.size();
    }

private:
    UnorderedMap<TKey, TValue, THash> key_value;
    Set<std::pair<TValue, TKey>> value_key;
};
