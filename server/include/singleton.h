#pragma once
#include <memory>
#include <mutex>

template <class T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    static inline std::shared_ptr<T> instance_ = nullptr;
public:
    static std::shared_ptr<T> Instance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            instance_ = std::shared_ptr<T>(new T);
        });
        return instance_;
    }
    virtual ~Singleton() = default;
};