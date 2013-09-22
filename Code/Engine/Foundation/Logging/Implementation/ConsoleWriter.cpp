#include <Foundation/PCH.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  static void SetConsoleColor (WORD ui)
  {
    SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), ui);
  }
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  static void SetConsoleColor (ezUInt8 ui) { }
#else
  #error Unknown Platform.
  static void SetConsoleColor (ezUInt8 ui) { }
#endif

void ezLogWriter::Console::LogMessageHandler(const ezLog::EventData& eventData)
{
  static ezMutex WriterLock; // will only be created if this writer is used at all
  ezLock<ezMutex> lock(WriterLock);

  if (eventData.m_EventType == ezLog::EventType::BeginGroup)
    printf ("\n");

  for (ezUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    printf (" ");

  switch (eventData.m_EventType)
  {
  case ezLog::EventType::FlushToDisk:
    break;
  case ezLog::EventType::BeginGroup:
    SetConsoleColor (0x02);
    printf ("+++++ %s +++++\n", eventData.m_szText);
    break;
  case ezLog::EventType::EndGroup:
    SetConsoleColor (0x02);
    printf ("----- %s -----\n\n", eventData.m_szText);
    break;
  case ezLog::EventType::FatalErrorMsg:
    SetConsoleColor (0x0D);
    printf ("Fatal Error: %s\n", eventData.m_szText);
    break;
  case ezLog::EventType::ErrorMsg:
    SetConsoleColor (0x0C);
    printf ("Error: %s\n", eventData.m_szText);
    break;
  case ezLog::EventType::SeriousWarningMsg:
    SetConsoleColor (0x0C);
    printf ("Seriously: %s\n", eventData.m_szText);
    break;
  case ezLog::EventType::WarningMsg:
    SetConsoleColor (0x0E);
    printf ("Warning: %s\n", eventData.m_szText);
    break;
  case ezLog::EventType::SuccessMsg:
    SetConsoleColor (0x0A);
    printf ("%s\n", eventData.m_szText);
    break;
  case ezLog::EventType::InfoMsg:
    SetConsoleColor (0x07);
    printf ("%s\n", eventData.m_szText);
    break;
  case ezLog::EventType::DevMsg:
    SetConsoleColor (0x08);
    printf ("%s\n", eventData.m_szText);
    break;
  case ezLog::EventType::DebugMsg:
  case ezLog::EventType::DebugRegularMsg:
    SetConsoleColor (0x09);
    printf ("%s\n", eventData.m_szText);
    break;
  default:
    SetConsoleColor (0x0D);
    printf ("%s\n", eventData.m_szText);

    ezLog::Warning ("Unknown Message Type %d", eventData.m_EventType);
    break;
  }

  SetConsoleColor (0x07);
}





