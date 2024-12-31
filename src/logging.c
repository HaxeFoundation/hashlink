
#include "hl.h"

static hl_log_handler logHandler;

HL_PRIM void hl_log_set_handler(hl_log_handler handler)
{
	logHandler = handler;
}

HL_PRIM void hl_log_printf(const char* source, int level, const char* format, ...)
{
	if (!logHandler) {
		return;
	}
	va_list args;
	va_start(args, format);
	logHandler(source, level, format, args);
	va_end(args);
}
