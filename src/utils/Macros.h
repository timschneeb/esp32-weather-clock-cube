//
// Created by tim on 25.09.25.
//

#ifndef MACROS_H
#define MACROS_H

#define SINGLETON(name) \
    public: \
        static name& instance() \
        { \
            static name instance; \
            return instance; \
        } \
        name(name const&) = delete; \
        void operator=(name const&)  = delete; \
    private: \
        name();

#endif //MACROS_H
