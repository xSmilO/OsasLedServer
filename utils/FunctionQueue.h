#pragma once
#include <queue>
#include <functional>
#include <tuple>
template<typename Func, typename... Args>
class FunctionCall {
private:
    Func func;
    std::tuple<Args...> args;
public:
    FunctionCall(Func f, Args... arguments) : func(f), args(std::make_tuple(arguments...)) {}

    void execute() const {
        std::apply(func, args);
    }
};


class FunctionQueue {
private:
    std::queue<std::function<void()>> queue;
public:
    template<typename Func, typename... Args>
    void push(Func f, Args... args) {
        auto functionCall = FunctionCall(f, args...);
        queue.push([functionCall]() {functionCall.execute();});
    }

    void execute() {
        if (queue.empty()) return;

        queue.front()();
        queue.pop();
    }
};