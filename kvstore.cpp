#include "kvstore.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>

KVStore::KVStore() {
    head = nullptr;
    transactionActive = false;
    transactionBackup = nullptr;

    loadFromDisk();
}

KVStore::~KVStore() {
    clearMemory();

    Entry* current = transactionBackup;

    while (current != nullptr) {
        Entry* next = current->next;
        delete current;
        current = next;
    }
}

void KVStore::clearMemory() {
    Entry* current = head;

    while (current != nullptr) {
        Entry* next = current->next;
        delete current;
        current = next;
    }

    head = nullptr;
}

Entry* KVStore::cloneList(Entry* source) {
    Entry* newHead = nullptr;
    Entry* tail = nullptr;

    while (source != nullptr) {
        Entry* copy = new Entry;

        copy->key = source->key;
        copy->value = source->value;
        copy->expireAt = source->expireAt;
        copy->next = nullptr;

        if (newHead == nullptr) {
            newHead = copy;
            tail = copy;
        } else {
            tail->next = copy;
            tail = copy;
        }

        source = source->next;
    }

    return newHead;
}

Entry* KVStore::findEntry(
    const std::string& key
) {
    Entry* current = head;

    while (current != nullptr) {
        if (current->key == key) {
            return current;
        }

        current = current->next;
    }

    return nullptr;
}

bool KVStore::isExpired(Entry* entry) {
    if (entry == nullptr ||
        entry->expireAt == 0) {
        return false;
    }

    long long now =
        static_cast<long long>(
            std::time(nullptr)
        );

    return now >= entry->expireAt;
}

void KVStore::setInMemory(
    const std::string& key,
    const std::string& value
) {
    Entry* existing = findEntry(key);

    if (existing != nullptr) {
        existing->value = value;
        existing->expireAt = 0;
        return;
    }

    Entry* entry = new Entry;

    entry->key = key;
    entry->value = value;
    entry->expireAt = 0;
    entry->next = head;

    head = entry;
}

bool KVStore::deleteInMemory(
    const std::string& key
) {
    Entry* current = head;
    Entry* previous = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
            if (previous == nullptr) {
                head = current->next;
            } else {
                previous->next =
                    current->next;
            }

            delete current;

            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}

void KVStore::appendLog(
    const std::string& line
) {
    if (transactionActive) {
        transactionLog.push_back(line);
        return;
    }

    std::ofstream file(
        "data.db",
        std::ios::app
    );

    if (file.is_open()) {
        file << line << "\n";
    }
}

void KVStore::set(
    const std::string& key,
    const std::string& value
) {
    setInMemory(key, value);

    appendLog(
        "SET " + key + " " + value
    );
}

bool KVStore::get(
    const std::string& key,
    std::string& value
) {
    Entry* entry = findEntry(key);

    if (entry == nullptr) {
        return false;
    }

    if (isExpired(entry)) {
        deleteInMemory(key);
        return false;
    }

    value = entry->value;

    return true;
}

bool KVStore::del(
    const std::string& key
) {
    Entry* entry = findEntry(key);

    if (entry != nullptr &&
        isExpired(entry)) {
        deleteInMemory(key);
        return false;
    }

    bool removed =
        deleteInMemory(key);

    if (removed) {
        appendLog(
            "DEL " + key
        );
    }

    return removed;
}

bool KVStore::exists(
    const std::string& key
) {
    Entry* entry = findEntry(key);

    if (entry == nullptr) {
        return false;
    }

    if (isExpired(entry)) {
        deleteInMemory(key);
        return false;
    }

    return true;
}

bool KVStore::expire(
    const std::string& key,
    long long seconds
) {
    Entry* entry = findEntry(key);

    if (entry == nullptr) {
        return false;
    }

    long long now =
        static_cast<long long>(
            std::time(nullptr)
        );

    entry->expireAt =
        now + seconds;

    appendLog(
        "EXPIREAT " +
        key +
        " " +
        std::to_string(
            entry->expireAt
        )
    );

    return true;
}

long long KVStore::ttl(
    const std::string& key
) {
    Entry* entry = findEntry(key);

    if (entry == nullptr) {
        return -2;
    }

    if (isExpired(entry)) {
        deleteInMemory(key);
        return -2;
    }

    if (entry->expireAt == 0) {
        return -1;
    }

    long long now =
        static_cast<long long>(
            std::time(nullptr)
        );

    return entry->expireAt - now;
}

void KVStore::flushDB() {
    clearMemory();

    if (transactionActive) {
        transactionLog.push_back(
            "FLUSHDB"
        );

        return;
    }

    std::ofstream file(
        "data.db",
        std::ios::trunc
    );
}

std::vector<std::string>
KVStore::range(
    const std::string& start,
    const std::string& end
) {
    std::vector<std::string> keys;

    Entry* current = head;

    while (current != nullptr) {
        bool internal =
            current->key.rfind(
                "@HASH@",
                0
            ) == 0 ||
            current->key.rfind(
                "@LIST@",
                0
            ) == 0;

        if (!internal &&
            !isExpired(current)) {

            bool afterStart =
                start.empty() ||
                current->key >= start;

            bool beforeEnd =
                end.empty() ||
                current->key <= end;

            if (afterStart &&
                beforeEnd) {
                keys.push_back(
                    current->key
                );
            }
        }

        current = current->next;
    }

    std::sort(
        keys.begin(),
        keys.end()
    );

    return keys;
}

bool KVStore::beginTransaction() {
    if (transactionActive) {
        return false;
    }

    transactionBackup =
        cloneList(head);

    transactionLog.clear();

    transactionActive = true;

    return true;
}

bool KVStore::commitTransaction() {
    if (!transactionActive) {
        return false;
    }

    bool containsFlush = false;

    for (
        const std::string& line :
        transactionLog
    ) {
        if (line == "FLUSHDB") {
            containsFlush = true;
            break;
        }
    }

    if (containsFlush) {
        std::ofstream clearFile(
            "data.db",
            std::ios::trunc
        );
    }

    std::ofstream file(
        "data.db",
        std::ios::app
    );

    if (file.is_open()) {
        for (
            const std::string& line :
            transactionLog
        ) {
            if (line != "FLUSHDB") {
                file
                    << line
                    << "\n";
            }
        }
    }

    Entry* current =
        transactionBackup;

    while (current != nullptr) {
        Entry* next = current->next;
        delete current;
        current = next;
    }

    transactionBackup = nullptr;
    transactionLog.clear();
    transactionActive = false;

    return true;
}

bool KVStore::abortTransaction() {
    if (!transactionActive) {
        return false;
    }

    clearMemory();

    head = transactionBackup;

    transactionBackup = nullptr;
    transactionLog.clear();
    transactionActive = false;

    return true;
}

std::string KVStore::makeHashKey(
    const std::string& hash,
    const std::string& field
) {
    return "@HASH@" +
           hash +
           ":" +
           field;
}

void KVStore::hset(
    const std::string& hash,
    const std::string& field,
    const std::string& value
) {
    std::string key =
        makeHashKey(
            hash,
            field
        );

    setInMemory(key, value);

    appendLog(
        "HSET " +
        hash +
        " " +
        field +
        " " +
        value
    );
}

bool KVStore::hget(
    const std::string& hash,
    const std::string& field,
    std::string& value
) {
    Entry* entry =
        findEntry(
            makeHashKey(
                hash,
                field
            )
        );

    if (entry == nullptr) {
        return false;
    }

    value = entry->value;

    return true;
}

std::vector<std::string>
KVStore::hgetall(
    const std::string& hash
) {
    std::vector<std::string> result;

    std::string prefix =
        "@HASH@" +
        hash +
        ":";

    Entry* current = head;

    while (current != nullptr) {
        if (
            current->key.rfind(
                prefix,
                0
            ) == 0
        ) {
            std::string field =
                current->key.substr(
                    prefix.length()
                );

            result.push_back(
                field +
                " " +
                current->value
            );
        }

        current = current->next;
    }

    std::sort(
        result.begin(),
        result.end()
    );

    return result;
}

std::string KVStore::makeListKey(
    const std::string& key
) {
    return "@LIST@" + key;
}

std::vector<std::string>
KVStore::splitList(
    const std::string& value
) {
    std::vector<std::string> result;

    if (value.empty()) {
        return result;
    }

    std::stringstream ss(value);

    std::string item;

    while (
        std::getline(
            ss,
            item,
            '\x1F'
        )
    ) {
        result.push_back(item);
    }

    return result;
}

std::string KVStore::joinList(
    const std::vector<std::string>& values
) {
    std::string result;

    for (
        std::size_t i = 0;
        i < values.size();
        i++
    ) {
        if (i > 0) {
            result += '\x1F';
        }

        result += values[i];
    }

    return result;
}

void KVStore::lpushInMemory(
    const std::string& key,
    const std::string& value
) {
    std::string internalKey =
        makeListKey(key);

    Entry* entry =
        findEntry(internalKey);

    std::vector<std::string> values;

    if (entry != nullptr) {
        values =
            splitList(
                entry->value
            );
    }

    values.insert(
        values.begin(),
        value
    );

    setInMemory(
        internalKey,
        joinList(values)
    );
}

void KVStore::rpushInMemory(
    const std::string& key,
    const std::string& value
) {
    std::string internalKey =
        makeListKey(key);

    Entry* entry =
        findEntry(internalKey);

    std::vector<std::string> values;

    if (entry != nullptr) {
        values =
            splitList(
                entry->value
            );
    }

    values.push_back(value);

    setInMemory(
        internalKey,
        joinList(values)
    );
}

bool KVStore::lpopInMemory(
    const std::string& key,
    std::string& value
) {
    std::string internalKey =
        makeListKey(key);

    Entry* entry =
        findEntry(internalKey);

    if (entry == nullptr) {
        return false;
    }

    std::vector<std::string> values =
        splitList(
            entry->value
        );

    if (values.empty()) {
        return false;
    }

    value = values.front();

    values.erase(
        values.begin()
    );

    if (values.empty()) {
        deleteInMemory(
            internalKey
        );
    } else {
        setInMemory(
            internalKey,
            joinList(values)
        );
    }

    return true;
}

void KVStore::lpush(
    const std::string& key,
    const std::string& value
) {
    lpushInMemory(key, value);

    appendLog(
        "LPUSH " +
        key +
        " " +
        value
    );
}

void KVStore::rpush(
    const std::string& key,
    const std::string& value
) {
    rpushInMemory(key, value);

    appendLog(
        "RPUSH " +
        key +
        " " +
        value
    );
}

std::vector<std::string>
KVStore::lrange(
    const std::string& key,
    int start,
    int stop
) {
    std::vector<std::string> result;

    Entry* entry =
        findEntry(
            makeListKey(key)
        );

    if (entry == nullptr) {
        return result;
    }

    std::vector<std::string> values =
        splitList(
            entry->value
        );

    int size =
        static_cast<int>(
            values.size()
        );

    if (start < 0) {
        start = size + start;
    }

    if (stop < 0) {
        stop = size + stop;
    }

    if (start < 0) {
        start = 0;
    }

    if (stop >= size) {
        stop = size - 1;
    }

    if (start >= size ||
        start > stop) {
        return result;
    }

    for (
        int i = start;
        i <= stop;
        i++
    ) {
        result.push_back(
            values[i]
        );
    }

    return result;
}

bool KVStore::lpop(
    const std::string& key,
    std::string& value
) {
    bool found =
        lpopInMemory(
            key,
            value
        );

    if (found) {
        appendLog(
            "LPOP " + key
        );
    }

    return found;
}

void KVStore::loadFromDisk() {
    std::ifstream file("data.db");

    if (!file.is_open()) {
        return;
    }

    std::string line;

    while (
        std::getline(
            file,
            line
        )
    ) {
        std::stringstream ss(line);

        std::string command;

        ss >> command;

        if (command == "SET") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            setInMemory(
                key,
                value
            );
        }

        else if (command == "DEL") {
            std::string key;

            ss >> key;

            deleteInMemory(key);
        }

        else if (command == "EXPIREAT") {
            std::string key;
            long long expireTime;

            ss >> key;
            ss >> expireTime;

            Entry* entry =
                findEntry(key);

            if (entry != nullptr) {
                entry->expireAt =
                    expireTime;
            }
        }

        else if (command == "HSET") {
            std::string hash;
            std::string field;
            std::string value;

            ss >> hash;
            ss >> field;
            ss >> value;

            setInMemory(
                makeHashKey(
                    hash,
                    field
                ),
                value
            );
        }

        else if (command == "LPUSH") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            lpushInMemory(
                key,
                value
            );
        }

        else if (command == "RPUSH") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            rpushInMemory(
                key,
                value
            );
        }

        else if (command == "LPOP") {
            std::string key;
            std::string ignored;

            ss >> key;

            lpopInMemory(
                key,
                ignored
            );
        }
    }
}
