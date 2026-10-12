/* FreeBSD-targeted libc++ uses 32-bit wchar_t. OpenOrbis libc's wmemchr
 * operates on 16-bit elements, including when libc++ optimizes std::find
 * over uint32_t arrays into that call. Keep the compiler's element ABI.
 */
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(__WCHAR_TYPE__) == 4, "This adapter requires 32-bit compiler wchar_t");

uint32_t *__wrap_wmemchr(const uint32_t *values, uint32_t value, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (values[i] == value)
            return (uint32_t *)(values + i);
    return NULL;
}
