#pragma once

template <typename EventData, typename MutexType>
ezEventBase<EventData, MutexType>::ezEventBase(ezIAllocator* pAllocator) : m_EventHandlers(pAllocator)
{
  m_bBroadcasting = false;
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::AddEventHandler(Handler handler)
{
  ezLock<MutexType> lock(m_Mutex);

  m_EventHandlers.PushBack(handler);
}

/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::RemoveEventHandler(Handler handler)
{
  ezLock<MutexType> lock(m_Mutex);

  bool bResult = m_EventHandlers.Remove(handler);  
  EZ_ASSERT(bResult, "ezEvent::RemoveEventHandler: Handler %p has not been registered or already been unregistered.", handler);
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::Broadcast(EventData eventData)
{
  ezLock<MutexType> lock(m_Mutex);

  EZ_ASSERT(!m_bBroadcasting, "The event has been triggered recursively or from several threads simultaneously.");  

  if (m_bBroadcasting)
    return;

  m_bBroadcasting = true;

  for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
    m_EventHandlers[ui](eventData);

  m_bBroadcasting = false;
}


template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent() : ezEventBase<EventData, MutexType>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent(ezIAllocator* pAllocator) : ezEventBase<EventData, MutexType>(pAllocator)
{
}
