#include "mapping_protocol.h"
#include <string.h>
#include <limits.h>
void s1_mapping_reset(S1MappingRequest *r)
{
    if (r != NULL)
    {
        memset(r, 0, sizeof(*r));
    }
}
bool s1_mapping_begin(S1MappingRequest *r, unsigned length)
{
    if (r == NULL)
    {
        return false;
    }
    s1_mapping_reset(r);
    if (length == 0u || length > S1_MAPPING_MAX)
    {
        return false;
    }
    r->expected = length;
    r->receiving = true;
    return true;
}
static void space(const char **p)
{
    while (**p == ' ' || **p == '\n' || **p == '\r' || **p == '\t')
    {
        ++*p;
    }
}
static bool token(const char **p, const char *value)
{
    space(p);
    unsigned n = (unsigned)strlen(value);
    if (strncmp(*p, value, n) != 0)
    {
        return false;
    }
    *p += n;
    return true;
}
static bool number(const char **p, uint32_t *value)
{
    space(p);
    const char *start = *p;
    uint32_t n = 0u;
    if (**p < '0' || **p > '9')
    {
        return false;
    }
    while (**p >= '0' && **p <= '9')
    {
        unsigned digit = (unsigned)(**p - '0');
        if (n > (UINT32_MAX - digit) / 10u)
        {
            return false;
        }
        n = n * 10u + digit;
        ++*p;
    }
    if (*start == '0' && *p - start > 1)
    {
        return false;
    }
    *value = n;
    return n != 0u;
}
static bool mapping(const char **p, barcode_action_t out[BARCODE_ALPHABET])
{
    static const char *const names[] = {"\"unmapped\"", "\"left\"", "\"right\"",
                                        "\"straight\"", "\"u_turn\""};
    if (!token(p, "["))
    {
        return false;
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        bool found = false;
        for (unsigned a = 0u; a < 5u; ++a)
        {
            const char *cursor = *p;
            if (token(&cursor, names[a]))
            {
                *p = cursor;
                out[i] = (barcode_action_t)a;
                found = true;
                break;
            }
        }
        if (!found || (i < BARCODE_ALPHABET - 1u && !token(p, ",")))
        {
            return false;
        }
    }
    return token(p, "]");
}
bool s1_mapping_data(S1MappingRequest *r, const void *data, unsigned length,
                     bool last)
{
    if (r == NULL)
    {
        return false;
    }
    if (!r->receiving || (data == NULL && length != 0u) ||
        length > r->expected - r->received)
    {
        r->receiving = false;
        return false;
    }
    if (length != 0u)
    {
        memcpy(r->payload + r->received, data, length);
    }
    r->received += length;
    if (!last)
    {
        return true;
    }
    r->receiving = false;
    if (r->received != r->expected || memchr(r->payload, '\0', r->received))
    {
        return false;
    }
    r->payload[r->received] = '\0';
    const char *p = r->payload;
    unsigned fields = 0u;
    if (!token(&p, "{"))
    {
        return false;
    }
    for (unsigned i = 0u; i < 3u; ++i)
    {
        unsigned field;
        if (token(&p, "\"request\""))
        {
            field = 1u;
            if (!token(&p, ":") || !token(&p, "\"barcode_mapping\""))
            {
                return false;
            }
        }
        else if (token(&p, "\"request_id\""))
        {
            field = 2u;
            if (!token(&p, ":") || !number(&p, &r->request_id))
            {
                return false;
            }
        }
        else if (token(&p, "\"mapping\""))
        {
            field = 4u;
            if (!token(&p, ":") || !mapping(&p, r->mapping))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        if ((fields & field) != 0u)
        {
            return false;
        }
        fields |= field;
        if (i < 2u && !token(&p, ","))
        {
            return false;
        }
    }
    if (fields != 7u || !token(&p, "}"))
    {
        return false;
    }
    space(&p);
    return *p == '\0';
}
