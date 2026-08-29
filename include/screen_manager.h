#ifndef MORRIS_BRIDGE_SCREEN_MANAGER_H
#define MORRIS_BRIDGE_SCREEN_MANAGER_H

/**
 * @file
 * Dispatches per-frame screen updates without a giant switch in main.cpp.
 */

#include "screen_types.h"

namespace screen
{

using UpdateFn = void (*)(void* context);

class Manager
{
public:
    void set_handler(Id screen, UpdateFn handler);
    void update(Id screen, void* context) const;

    [[nodiscard]] static int index(Id screen);

private:
    UpdateFn _handlers[9] = {};
};

} // namespace screen

#endif
