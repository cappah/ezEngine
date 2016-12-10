#pragma once

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

class ezStringBuilder;
class ezStringView;

class EZ_FOUNDATION_DLL ezFormatString
{
public:
  ezFormatString() { m_szString = nullptr; }
  ezFormatString(const char* szString) { m_szString = szString; }
  virtual ~ezFormatString() {}

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an ezStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a const char* to that
  /// string builder data for convenience.
  virtual const char* GetText(ezStringBuilder& sb) const { return m_szString; }

protected:
  // out of line function so that we don't need to include ezStringBuilder here, to break include dependency cycle
  static void SBAppendView(ezStringBuilder& sb, const ezStringView& sub);
  static void SBClear(ezStringBuilder& sb);
  static void SBAppendChar(ezStringBuilder& sb, ezUInt32 uiChar);
  static const char* SBReturn(ezStringBuilder& sb);

  const char* m_szString;
};

#include <Foundation/Strings/Implementation/FormatStringImpl.h>

template<typename ... ARGS>
ezFormatStringImpl<ARGS...> ezFmt(const char* szFormat, ARGS... args)
{
  return ezFormatStringImpl<ARGS...>(szFormat, args...);
}
