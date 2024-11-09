//
// Created by decryp7 on 9/11/24.
//

#ifndef NOTIFIER_POOL_H
#define NOTIFIER_POOL_H
#include <iostream>
#include <queue>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>
#include <boost/optional/optional.hpp>

class notifier_pool;
class scoped_notifier;

class notifier {
    friend class scoped_notifier;
public:
    explicit notifier(std::string  id, notifier_pool *parent) : id(std::move(id)), parent{parent}{}
    std::string get_id() const { return id; }
private:
    std::string id;
    notifier_pool* parent{};
};

class scoped_notifier {
    friend class notifier_pool;
public:
    scoped_notifier() = delete;
    explicit scoped_notifier(notifier *notifier) : notifier{notifier}{}
    std::string get_id() const { return notifier->get_id(); }
    ~scoped_notifier();
private:
    notifier *notifier;
};

class notifier_pool {
    friend class scoped_notifier;
public:
    notifier_pool() : free{}, lock{} {
        std::lock_guard<std::mutex> guard(lock);
        for(int i=0;i<10;i++) {
            std::stringstream ss{};
            ss << "T" << i;
            free.push(new notifier(ss.str(), this));
        }
    }

    notifier *get_notifier() {
        notifier *ret = nullptr;
        do {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        }while ([&] {
            std::lock_guard<std::mutex> guard(lock);
            if(!free.empty()) {
                ret = free.front();
                free.pop();
                return false;
            }
            return true;
        }());

        return ret;
    }

private:
    std::queue<notifier*> free;
    std::mutex lock;
    int total;

    void return_notifier(notifier *notifier) {
        std::lock_guard<std::mutex> guard(lock);
        free.push(notifier);
    }
};

inline scoped_notifier::~scoped_notifier() {
    if(notifier != nullptr) {
        notifier->parent->return_notifier(notifier);
    }
}


#endif //NOTIFIER_POOL_H
