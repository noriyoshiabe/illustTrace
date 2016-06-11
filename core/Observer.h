#pragma once

#include <cstdarg>

namespace illustrace {
namespace core {

template <class C, typename E>
class Observer {
public:
    Observer() {};
    virtual ~Observer() {};
    virtual void notify(C *sender, E event, va_list argList) = 0;
};

} // namespace core
} // namespace illustrace