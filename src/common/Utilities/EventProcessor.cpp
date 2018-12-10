
#include "EventProcessor.h"
#include "Errors.h"

void BasicEvent::ScheduleAbort()
{
    ASSERT(IsRunning()
        && "Tried to scheduled the abortion of an event twice!");
    m_abortState = AbortState::STATE_ABORT_SCHEDULED;
}

void BasicEvent::SetAborted()
{
    ASSERT(!IsAborted()
        && "Tried to abort an already aborted event!");
    m_abortState = AbortState::STATE_ABORTED;
}


EventProcessor::EventProcessor()
{
    m_time = 0;
    m_aborting = false;
}

EventProcessor::~EventProcessor()
{
    KillAllEvents(true);
}

void EventProcessor::Update(uint32 p_time)
{
    // update time
    m_time += p_time;

    // main event loop
    EventList::iterator i;
    while (((i = m_events.begin()) != m_events.end()) && i->first <= m_time)
    {
        // get and remove event from queue
        BasicEvent* event = i->second;
        m_events.erase(i);

        if (event->IsRunning())
        {
            if (event->Execute(m_time, p_time))
            {
                // completely destroy event if it is not re-added
                delete event;
            }
            continue;
        }

        if (event->IsAbortScheduled())
        {
            event->Abort(m_time);
            // Mark the event as aborted
            event->SetAborted();
        }

        if (event->IsDeletable())
        {
            delete event;
            continue;
        }

        // Reschedule non deletable events to be checked at
        // the next update tick
        AddEvent(event, CalculateTime(1), false);
    }
}

void EventProcessor::KillAllEvents(bool force)
{
    // prevent event insertions
    m_aborting = true;

    // first, abort all existing events
    for (auto itr = m_events.begin(); itr != m_events.end();)
    {
        // Abort events which weren't aborted already
        if (!itr->second->IsAborted())
        {
            itr->second->SetAborted();
            itr->second->Abort(m_time);
        }

        // Skip non-deletable events when we are
        // not forcing the event cancellation.
        if (!force && !itr->second->IsDeletable())
        {
            ++itr;
            continue;
        }

        delete itr->second;

        if (force)
            ++itr; // Clear the whole container when forcing
        else
            itr = m_events.erase(itr);
    }

    // fast clear event list (in force case)
    if (force)
        m_events.clear();
}

void EventProcessor::AddEvent(BasicEvent* Event, uint64 e_time, bool set_addtime)
{
    if (set_addtime)
        Event->m_addTime = m_time;
    Event->m_execTime = e_time;
    m_events.insert(std::pair<uint64, BasicEvent*>(e_time, Event));
}

void EventProcessor::ModifyEventTime(BasicEvent* Event, uint64 newTime)
{
    for (auto itr = m_events.begin(); itr != m_events.end(); ++itr)
    {
        if (itr->second != Event)
            continue;

        Event->m_execTime = newTime;
        m_events.erase(itr);
        m_events.insert(std::pair<uint64, BasicEvent*>(newTime, Event));
        break;
    }
}

uint64 EventProcessor::CalculateTime(uint64 t_offset) const
{
    return(m_time + t_offset);
}

uint64 EventProcessor::CalculateQueueTime(uint64 delay) const
{
    return CalculateTime(delay - (m_time % delay));
}
