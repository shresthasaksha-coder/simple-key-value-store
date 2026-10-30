#ifndef KVSTORE_H
#define KVSTORE_H

#include <string>
#include <vector>

struct Entry {
    std::string key;
    std::string value;
    long long expireAt;
    Entry* next;
};

class KVStore {
private:
    Entry* head;

    bool transactionActive;
    Entry* transactionBackup;
    std::vector<std::string> transactionLog;

    Entry* findEntry(const std::string& key);
    void loadFromDisk();

    void setInMemory(
        const std::string& key,
        const std::string& value
    );

    bool deleteInMemory(
        const std::string& key
    );

    bool isExpired(Entry* entry);
    void clearMemory();

    Entry* cloneList(Entry* source);

    void appendLog(
        const std::string& line
    );

    std::string makeHashKey(
        const std::string& hash,
        const std::string& field
    );

    std::string makeListKey(
        const std::string& key
    );

    std::vector<std::string> splitList(
        const std::string& value
    );

    std::string joinList(
        const std::vector<std::string>& values
    );

    void lpushInMemory(
        const std::string& key,
        const std::string& value
    );

    void rpushInMemory(
        const std::string& key,
        const std::string& value
    );

    bool lpopInMemory(
        const std::string& key,
        std::string& value
    );

public:
    KVStore();
    ~KVStore();

    void set(
        const std::string& key,
        const std::string& value
    );

    bool get(
        const std::string& key,
        std::string& value
    );

    bool del(const std::string& key);
    bool exists(const std::string& key);

    bool expire(
        const std::string& key,
        long long seconds
    );

    long long ttl(
        const std::string& key
    );

    void flushDB();

    std::vector<std::string> range(
        const std::string& start,
        const std::string& end
    );

    bool beginTransaction();
    bool commitTransaction();
    bool abortTransaction();

    void hset(
        const std::string& hash,
        const std::string& field,
        const std::string& value
    );

    bool hget(
        const std::string& hash,
        const std::string& field,
        std::string& value
    );

    std::vector<std::string> hgetall(
        const std::string& hash
    );

    void lpush(
        const std::string& key,
        const std::string& value
    );

    void rpush(
        const std::string& key,
        const std::string& value
    );

    std::vector<std::string> lrange(
        const std::string& key,
        int start,
        int stop
    );

    bool lpop(
        const std::string& key,
        std::string& value
    );
};

#endif
