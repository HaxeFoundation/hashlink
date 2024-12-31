
#include "hl.h"

static hl_event_handler eventHandler;

HL_PRIM void hl_event_set_handler(hl_event_handler handler)
{
	eventHandler = handler;
}

HL_PRIM void hl_event(int eventId, void* data)
{
	if (!eventHandler)
	{
		return;
	}
	eventHandler(eventId, data);
}