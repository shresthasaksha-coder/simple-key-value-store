#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "kvstore.h"

int main() {
    std::remove("data.db");

    KVStore store;

    // SET / GET
    store.set("name", "Saksham");

    std::string value;

    assert(store.get("name", value));
    assert(value == "Saksham");

    // EXISTS / DEL
    assert(store.exists("name"));

    assert(store.del("name"));

    assert(!store.exists("name"));
    assert(!store.get("name", value));

    // INCR-style behavior using SET / GET
    store.set("counter", "1");

    assert(store.get("counter", value));
    assert(value == "1");

    // HASH
    store.hset(
        "person",
        "field1",
        "value1"
    );

    assert(
        store.hget(
            "person",
            "field1",
            value
        )
    );

    assert(value == "value1");

    // LIST
    store.lpush(
        "mylist",
        "item1"
    );

    store.rpush(
        "mylist",
        "item2"
    );

    store.lpush(
        "mylist",
        "item3"
    );

    std::vector<std::string> list =
        store.lrange(
            "mylist",
            0,
            -1
        );

    assert(list.size() == 3);
    assert(list[0] == "item3");
    assert(list[1] == "item1");
    assert(list[2] == "item2");

    assert(
        store.lpop(
            "mylist",
            value
        )
    );

    assert(value == "item3");

    // TRANSACTION ABORT
    assert(
        store.beginTransaction()
    );

    store.set(
        "temporary",
        "hello"
    );

    assert(
        store.get(
            "temporary",
            value
        )
    );

    assert(value == "hello");

    assert(
        store.abortTransaction()
    );

    assert(
        !store.get(
            "temporary",
            value
        )
    );

    // TRANSACTION COMMIT
    assert(
        store.beginTransaction()
    );

    store.set(
        "saved",
        "yes"
    );

    assert(
        store.commitTransaction()
    );

    assert(
        store.get(
            "saved",
            value
        )
    );

    assert(value == "yes");

    std::cout
        << "All tests passed!"
        << std::endl;

    std::remove("data.db");

    return 0;
}
