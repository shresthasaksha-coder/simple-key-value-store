#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "kvstore.h"

int main() {
    KVStore store;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);

        std::string command;
        ss >> command;

        if (command == "SET") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            store.set(key, value);

            std::cout << "OK" << std::endl;
        }

        else if (command == "GET") {
            std::string key;
            std::string value;

            ss >> key;

            if (store.get(key, value)) {
                std::cout
                    << value
                    << std::endl;
            } else {
                std::cout
                    << "nil"
                    << std::endl;
            }
        }

        else if (command == "DEL") {
            std::string key;

            ss >> key;

            std::cout
                << (store.del(key)
                    ? "1"
                    : "0")
                << std::endl;
        }

        else if (command == "EXISTS") {
            std::string key;

            ss >> key;

            std::cout
                << (store.exists(key)
                    ? "1"
                    : "0")
                << std::endl;
        }

        else if (command == "MSET") {
            std::string key;
            std::string value;

            bool valid = true;

            while (ss >> key) {
                if (!(ss >> value)) {
                    valid = false;
                    break;
                }

                store.set(
                    key,
                    value
                );
            }

            std::cout
                << (valid
                    ? "OK"
                    : "ERROR")
                << std::endl;
        }

        else if (command == "MGET") {
            std::string key;

            while (ss >> key) {
                std::string value;

                if (store.get(
                        key,
                        value)) {
                    std::cout
                        << value
                        << std::endl;
                } else {
                    std::cout
                        << "nil"
                        << std::endl;
                }
            }
        }

        else if (command == "INCR" ||
                 command == "DECR") {
            std::string key;
            std::string value;

            ss >> key;

            long long number = 0;

            if (store.get(key, value)) {
                try {
                    number =
                        std::stoll(value);
                }
                catch (...) {
                    std::cout
                        << "ERROR"
                        << std::endl;

                    continue;
                }
            }

            if (command == "INCR") {
                number++;
            } else {
                number--;
            }

            store.set(
                key,
                std::to_string(number)
            );

            std::cout
                << number
                << std::endl;
        }

        else if (command == "EXPIRE") {
            std::string key;
            long long seconds;

            ss >> key;
            ss >> seconds;

            std::cout
                << (store.expire(
                        key,
                        seconds)
                    ? "1"
                    : "0")
                << std::endl;
        }

        else if (command == "TTL") {
            std::string key;

            ss >> key;

            std::cout
                << store.ttl(key)
                << std::endl;
        }

        else if (command == "FLUSHDB") {
            store.flushDB();

            std::cout
                << "OK"
                << std::endl;
        }

        else if (command == "RANGE") {
            std::string start;
            std::string end;

            ss >> start;
            ss >> end;

            if (start == "\"\"") {
                start = "";
            }

            if (end == "\"\"") {
                end = "";
            }

            std::vector<std::string> keys =
                store.range(
                    start,
                    end
                );

            for (
                const std::string& key :
                keys
            ) {
                std::cout
                    << key
                    << std::endl;
            }

            std::cout
                << "END"
                << std::endl;
        }

        else if (command == "BEGIN") {
            std::cout
                << (store.beginTransaction()
                    ? "OK"
                    : "ERROR")
                << std::endl;
        }

        else if (command == "COMMIT") {
            std::cout
                << (store.commitTransaction()
                    ? "OK"
                    : "ERROR")
                << std::endl;
        }

        else if (command == "ABORT") {
            std::cout
                << (store.abortTransaction()
                    ? "OK"
                    : "ERROR")
                << std::endl;
        }

        else if (command == "HSET") {
            std::string hash;
            std::string field;
            std::string value;

            ss >> hash;
            ss >> field;
            ss >> value;

            store.hset(
                hash,
                field,
                value
            );

            std::cout
                << "OK"
                << std::endl;
        }

        else if (command == "HGET") {
            std::string hash;
            std::string field;
            std::string value;

            ss >> hash;
            ss >> field;

            if (store.hget(
                    hash,
                    field,
                    value)) {
                std::cout
                    << value
                    << std::endl;
            } else {
                std::cout
                    << "nil"
                    << std::endl;
            }
        }

        else if (command == "HGETALL") {
            std::string hash;

            ss >> hash;

            std::vector<std::string> values =
                store.hgetall(hash);

            for (
                const std::string& value :
                values
            ) {
                std::cout
                    << value
                    << std::endl;
            }
        }

        else if (command == "LPUSH") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            store.lpush(
                key,
                value
            );

            std::cout
                << "OK"
                << std::endl;
        }

        else if (command == "RPUSH") {
            std::string key;
            std::string value;

            ss >> key;
            ss >> value;

            store.rpush(
                key,
                value
            );

            std::cout
                << "OK"
                << std::endl;
        }

        else if (command == "LRANGE") {
            std::string key;
            int start;
            int stop;

            ss >> key;
            ss >> start;
            ss >> stop;

            std::vector<std::string> values =
                store.lrange(
                    key,
                    start,
                    stop
                );

            for (
                const std::string& value :
                values
            ) {
                std::cout
                    << value
                    << std::endl;
            }
        }

        else if (command == "LPOP") {
            std::string key;
            std::string value;

            ss >> key;

            if (store.lpop(
                    key,
                    value)) {
                std::cout
                    << value
                    << std::endl;
            } else {
                std::cout
                    << "nil"
                    << std::endl;
            }
        }

        else if (command == "EXIT") {
            break;
        }
    }

    return 0;
}
