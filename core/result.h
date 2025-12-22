#pragma once
#include <string>
#include <utility>

struct Status {
    bool ok = true;
    std::string error;

    static Status Ok() { return {true, {}}; }
    static Status Fail(std::string msg) { return {false, std::move(msg)}; }
};

template <class T>
struct Result {
    bool ok = true;
    std::string error;
    T value{};

    static Result<T> Ok(T v) {
        Result<T> r;
        r.ok = true;
        r.value = std::move(v);
        return r;
    }

    static Result<T> Fail(std::string msg) {
        Result<T> r;
        r.ok = false;
        r.error = std::move(msg);
        return r;
    }
};
