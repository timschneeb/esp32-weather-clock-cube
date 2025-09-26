//
// Created by tim on 25.09.25.
//

#ifndef EVENTMACROS_H
#define EVENTMACROS_H

// Preprocessor utilities
#define EXPAND(x) x
#define CAT(a, b) a##b

// Count arguments (supports up to 10 tuples)
#define COUNT_ARGS(...) EXPAND(COUNT_ARGS_IMPL(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1))
#define COUNT_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N

// FOR_EACH implementations (up to 10 args)
#define FOR_EACH_1(WHAT, X) WHAT X
#define FOR_EACH_2(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_1(WHAT, __VA_ARGS__))
#define FOR_EACH_3(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_2(WHAT, __VA_ARGS__))
#define FOR_EACH_4(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_3(WHAT, __VA_ARGS__))
#define FOR_EACH_5(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_4(WHAT, __VA_ARGS__))
#define FOR_EACH_6(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_5(WHAT, __VA_ARGS__))
#define FOR_EACH_7(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_6(WHAT, __VA_ARGS__))
#define FOR_EACH_8(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_7(WHAT, __VA_ARGS__))
#define FOR_EACH_9(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_8(WHAT, __VA_ARGS__))
#define FOR_EACH_10(WHAT, X, ...) WHAT X EXPAND(FOR_EACH_9(WHAT, __VA_ARGS__))

#define FOR_EACH_N(N, WHAT, ...) EXPAND(CAT(FOR_EACH_, N)(WHAT, __VA_ARGS__))
#define FOR_EACH(WHAT, ...) EXPAND(FOR_EACH_N(COUNT_ARGS(__VA_ARGS__), WHAT, __VA_ARGS__))

// Field generator helpers
#define DECL_MEMBER(type, name) type _##name;
#define DECL_GETTER(type, name) type name() const { return _##name; }
#define INIT_MEMBER(type, name) this->_##name = name;
#define CONSTRUCTOR_ARG(type, name) const type& name,

// Main macro to register event args class
#define REGISTER_EVENT(name, ...) \
    class name##Event : public IEvent { \
    public: \
        explicit name##Event( \
            FOR_EACH(CONSTRUCTOR_ARG, __VA_ARGS__) int dummy=0 \
        ) : IEvent(EventId::name) { \
            FOR_EACH(INIT_MEMBER, __VA_ARGS__) \
        } \
        FOR_EACH(DECL_GETTER, __VA_ARGS__) \
    private: \
        FOR_EACH(DECL_MEMBER, __VA_ARGS__) \
    };

#define REGISTER_EVENT_NOARGS(name) \
    class name##Event : public IEvent { \
    public: \
        explicit name##Event() : IEvent(EventId::name) {} \
    };

#endif //EVENTMACROS_H
