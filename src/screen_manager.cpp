#include "screen_manager.h"

namespace screen
{

int Manager::index(const Id screen)
{
    return int(screen);
}

void Manager::set_handler(const Id screen, const UpdateFn handler)
{
    const int i = index(screen);

    if(i >= 0 && i < 9)
    {
        _handlers[i] = handler;
    }
}

void Manager::update(const Id screen, void* context) const
{
    const int i = index(screen);

    if(i >= 0 && i < 9)
    {
        const UpdateFn handler = _handlers[i];

        if(handler)
        {
            handler(context);
        }
    }
}

} // namespace screen
