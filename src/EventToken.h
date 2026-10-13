#pragma once
#include <windows.h>
// Windows SDK ABI type used by WebView2. MinGW does not ship EventToken.h.
typedef struct EventRegistrationToken { INT64 value; } EventRegistrationToken;
static_assert(sizeof(EventRegistrationToken) == 8);
