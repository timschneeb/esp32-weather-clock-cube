//
// Created by tim on 25.09.25.
//

#ifndef ATOMICVALUE_H
#define ATOMICVALUE_H

#include <mutex>

template<typename T>
class AtomicValue {
    T value;
    mutable std::mutex mtx;

public:
    AtomicValue() = default;
    // ReSharper disable once CppNonExplicitConvertingConstructor
    AtomicValue(const T& v) : value(v) {}

    void store(const T& v) {
        std::lock_guard<std::mutex> lock(mtx);
        value = v;
    }

    T load() const {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }

    AtomicValue& operator=(const T& v) {
        store(v);
        return *this;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator T() const {
        return load();
    }
};


#endif //ATOMICVALUE_H
