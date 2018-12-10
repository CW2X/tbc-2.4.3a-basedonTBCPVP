/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "Common.h"
#include "Containers.h"
#include "Random.h"

#include <string>
#include <vector>

typedef std::vector<std::string> Tokens;

TC_COMMON_API Tokens StrSplit(const std::string &src, const std::string &sep);

TC_COMMON_API int32 MoneyStringToMoney(const std::string& moneyString);

TC_COMMON_API struct tm* localtime_r(const time_t* time, struct tm *result);

TC_COMMON_API std::string secsToTimeString(uint32 timeInSecs, bool shortText = false, bool hoursOnly = false);
TC_COMMON_API uint32 TimeStringToSecs(const std::string& timestring);
TC_COMMON_API std::string TimeToTimestampStr(time_t t);

inline uint32 secsToTimeBitFields(time_t secs)
{
    tm* lt = localtime(&secs);
    return (lt->tm_year - 100) << 24 | lt->tm_mon  << 20 | (lt->tm_mday - 1) << 14 | lt->tm_wday << 11 | lt->tm_hour << 6 | lt->tm_min;
}

inline void ApplyPercentModFloatVar(float& var, float val, bool apply)
{
    if (val == -100.0f)     // prevent set var to zero
        val = -99.99f;
    var *= (apply ? (100.0f + val) / 100.0f : 100.0f / (100.0f + val));
}

// Percentage calculation
template <class T, class U>
inline T CalculatePct(T base, U pct)
{
    return T(base * static_cast<float>(pct) / 100.0f);
}

template <class T, class U>
inline T AddPct(T &base, U pct)
{
    return base += CalculatePct(base, pct);
}

template <class T, class U>
inline T ApplyPct(T &base, U pct)
{
    return base = CalculatePct(base, pct);
}

template <class T>
inline T RoundToInterval(T& num, T floor, T ceil)
{
    return num = std::min(std::max(num, floor), ceil);
}

inline void ApplyModUInt32Var(uint32& var, int32 val, bool apply)
{
    int32 cur = var;
    cur += (apply ? val : -val);
    if(cur < 0)
        cur = 0;
    var = cur;
}

inline void ApplyModFloatVar(float& var, float  val, bool apply)
{
    var += (apply ? val : -val);
    if(var < 0)
        var = 0;
}

template <class T>
inline T square(T x) { return x * x; }

TC_COMMON_API bool Utf8toWStr(const std::string& utf8str, std::wstring& wstr);
// in wsize==max size of buffer, out wsize==real string size
TC_COMMON_API bool Utf8toWStr(char const* utf8str, size_t csize, wchar_t* wstr, size_t& wsize);

inline bool Utf8toWStr(const std::string& utf8str, wchar_t* wstr, size_t& wsize)
{
    return Utf8toWStr(utf8str.c_str(), utf8str.size(), wstr, wsize);
}

TC_COMMON_API bool WStrToUtf8(std::wstring wstr, std::string& utf8str);
// size==real string size
TC_COMMON_API bool WStrToUtf8(wchar_t* wstr, size_t size, std::string& utf8str);

TC_COMMON_API size_t utf8length(std::string& utf8str);                    // set string to "" if invalid utf8 sequence
TC_COMMON_API void utf8truncate(std::string& utf8str,size_t len);

inline bool isBasicLatinCharacter(wchar_t wchar)
{
    if(wchar >= L'a' && wchar <= L'z')                      // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return true;
    if(wchar >= L'A' && wchar <= L'Z')                      // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return true;
    return false;
}

inline bool isExtendedLatinCharacter(wchar_t wchar)
{
    if(isBasicLatinCharacter(wchar))
        return true;
    if(wchar >= 0x00C0 && wchar <= 0x00D6)                  // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return true;
    if(wchar >= 0x00D8 && wchar <= 0x00DF)                  // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return true;
    if(wchar == 0x00DF)                                     // LATIN SMALL LETTER SHARP S
        return true;
    if(wchar >= 0x00E0 && wchar <= 0x00F6)                  // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return true;
    if(wchar >= 0x00F8 && wchar <= 0x00FE)                  // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return true;
    if(wchar >= 0x0100 && wchar <= 0x012F)                  // LATIN CAPITAL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK
        return true;
    if(wchar == 0x1E9E)                                     // LATIN CAPITAL LETTER SHARP S
        return true;
    return false;
}

inline bool isCyrillicCharacter(wchar_t wchar)
{
    if(wchar >= 0x0410 && wchar <= 0x044F)                  // CYRILLIC CAPITAL LETTER A - CYRILLIC SMALL LETTER YA
        return true;
    if(wchar == 0x0401 || wchar == 0x0451)                  // CYRILLIC CAPITAL LETTER IO, CYRILLIC SMALL LETTER IO
        return true;
    return false;
}

inline bool isEastAsianCharacter(wchar_t wchar)
{
    if(wchar >= 0x1100 && wchar <= 0x11F9)                  // Hangul Jamo
        return true;
    if(wchar >= 0x3041 && wchar <= 0x30FF)                  // Hiragana + Katakana
        return true;
    if(wchar >= 0x3131 && wchar <= 0x318E)                  // Hangul Compatibility Jamo
        return true;
    if(wchar >= 0x31F0 && wchar <= 0x31FF)                  // Katakana Phonetic Ext.
        return true;
    if(wchar >= 0x3400 && wchar <= 0x4DB5)                  // CJK Ideographs Ext. A
        return true;
    if(wchar >= 0x4E00 && wchar <= 0x9FC3)                  // Unified CJK Ideographs
        return true;
    if(wchar >= 0xAC00 && wchar <= 0xD7A3)                  // Hangul Syllables
        return true;
    if(wchar >= 0xFF01 && wchar <= 0xFFEE)                  // Halfwidth forms
        return true;
    return false;
}

inline bool isNumeric(wchar_t wchar)
{
    return (wchar >= L'0' && wchar <=L'9');
}

inline bool isNumeric(char c)
{
    return (c >= '0' && c <='9');
}

inline bool isNumericOrSpace(wchar_t wchar)
{
    return isNumeric(wchar) || wchar == L' ';
}

inline bool isBasicLatinString(std::wstring wstr, bool numericOrSpace)
{
    for(wchar_t i : wstr)
        if(!isBasicLatinCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isExtendedLatinString(std::wstring wstr, bool numericOrSpace)
{
    for(wchar_t i : wstr)
        if(!isExtendedLatinCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isCyrillicString(std::wstring wstr, bool numericOrSpace)
{
    for(wchar_t i : wstr)
        if(!isCyrillicCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isEastAsianString(std::wstring wstr, bool numericOrSpace)
{
    for(wchar_t i : wstr)
        if(!isEastAsianCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline wchar_t wcharToUpper(wchar_t wchar)
{
    if(wchar >= L'a' && wchar <= L'z')                      // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return wchar_t(uint16(wchar)-0x0020);
    if(wchar == 0x00DF)                                     // LATIN SMALL LETTER SHARP S
        return wchar_t(0x1E9E);
    if(wchar >= 0x00E0 && wchar <= 0x00F6)                  // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return wchar_t(uint16(wchar)-0x0020);
    if(wchar >= 0x00F8 && wchar <= 0x00FE)                  // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return wchar_t(uint16(wchar)-0x0020);
    if(wchar >= 0x0101 && wchar <= 0x012F)                  // LATIN SMALL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK (only %2=1)
    {
        if(wchar % 2 == 1)
            return wchar_t(uint16(wchar)-0x0001);
    }
    if(wchar >= 0x0430 && wchar <= 0x044F)                  // CYRILLIC SMALL LETTER A - CYRILLIC SMALL LETTER YA
        return wchar_t(uint16(wchar)-0x0020);
    if(wchar == 0x0451)                                     // CYRILLIC SMALL LETTER IO
        return wchar_t(0x0401);

    return wchar;
}

inline wchar_t wcharToUpperOnlyLatin(wchar_t wchar)
{
    return isBasicLatinCharacter(wchar) ? wcharToUpper(wchar) : wchar;
}

inline wchar_t wcharToLower(wchar_t wchar)
{
    if(wchar >= L'A' && wchar <= L'Z')                      // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return wchar_t(uint16(wchar)+0x0020);
    if(wchar >= 0x00C0 && wchar <= 0x00D6)                  // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return wchar_t(uint16(wchar)+0x0020);
    if(wchar >= 0x00D8 && wchar <= 0x00DE)                  // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return wchar_t(uint16(wchar)+0x0020);
    if(wchar >= 0x0100 && wchar <= 0x012E)                  // LATIN CAPITAL LETTER A WITH MACRON - LATIN CAPITAL LETTER I WITH OGONEK (only %2=0)
    {
        if(wchar % 2 == 0)
            return wchar_t(uint16(wchar)+0x0001);
    }
    if(wchar == 0x1E9E)                                     // LATIN CAPITAL LETTER SHARP S
        return wchar_t(0x00DF);
    if(wchar == 0x0401)                                     // CYRILLIC CAPITAL LETTER IO
        return wchar_t(0x0451);
    if(wchar >= 0x0410 && wchar <= 0x042F)                  // CYRILLIC CAPITAL LETTER A - CYRILLIC CAPITAL LETTER YA
        return wchar_t(uint16(wchar)+0x0020);

    return wchar;
}

inline void wstrToUpper(std::wstring& str)
{
    std::transform( str.begin(), str.end(), str.begin(), wcharToUpper );
}

inline void wstrToLower(std::wstring& str)
{
    std::transform( str.begin(), str.end(), str.begin(), wcharToLower );
}

TC_COMMON_API std::wstring GetMainPartOfName(std::wstring wname, uint32 declension);

TC_COMMON_API bool utf8ToConsole(const std::string& utf8str, std::string& conStr);
TC_COMMON_API bool consoleToUtf8(const std::string& conStr,std::string& utf8str);
TC_COMMON_API bool Utf8FitTo(const std::string& str, std::wstring search);
TC_COMMON_API void utf8printf(FILE* out, const char *str, ...);
TC_COMMON_API void vutf8printf(FILE* out, const char *str, va_list* ap);

/* Select a random element from a container. Note: make sure you explicitly empty check the container */
template <class C> typename C::value_type const& SelectRandomContainerElement(C const& container)
{
    typename C::const_iterator it = container.begin();
    std::advance(it, urand(0, container.size() - 1));
    return *it;
}

// simple class for not-modifyable list
template <typename T>
class HookList final
{
	typedef std::vector<T> ContainerType;

private:
	ContainerType _container;

public:
	typedef typename ContainerType::iterator iterator;
    
    HookList<T>& operator+=(T&& t)
    {
        _container.push_back(std::move(t));
        return *this;
    }
    HookList<T> & operator-=(T t)
    {
		_container.remove(t);
        return *this;
    }
    size_t size() const
    {
        return _container.size();
    }
	iterator begin()
    {
        return _container.begin();
    }
	iterator end()
    {
        return _container.end();
    }
};

#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
#define UTF8PRINTF(OUT,FRM,RESERR)                      \
{                                                       \
    char temp_buf[6000];                                \
    va_list ap;                                         \
    va_start(ap, FRM);                                  \
    size_t temp_len = vsnprintf(temp_buf,6000,FRM,ap);  \
    va_end(ap);                                         \
                                                        \
    wchar_t wtemp_buf[6000];                            \
    size_t wtemp_len = 6000-1;                          \
    if(!Utf8toWStr(temp_buf,temp_len,wtemp_buf,wtemp_len)) \
        return RESERR;                                  \
    CharToOemBuffW(&wtemp_buf[0],&temp_buf[0],wtemp_len+1);\
    fprintf(OUT,temp_buf);                              \
}
#else
#define UTF8PRINTF(OUT,FRM,RESERR)                      \
{                                                       \
    va_list ap;                                         \
    va_start(ap, FRM);                                  \
    vfprintf(OUT, FRM, ap );                            \
    va_end(ap);                                         \
}
#endif

enum ComparisionType
{
    COMP_TYPE_EQ = 0,
    COMP_TYPE_HIGH,
    COMP_TYPE_LOW,
    COMP_TYPE_HIGH_EQ,
    COMP_TYPE_LOW_EQ,
    COMP_TYPE_MAX
};

template <class T>
bool CompareValues(ComparisionType type, T val1, T val2)
{
    switch (type)
    {
        case COMP_TYPE_EQ:
            return val1 == val2;
        case COMP_TYPE_HIGH:
            return val1 > val2;
        case COMP_TYPE_LOW:
            return val1 < val2;
        case COMP_TYPE_HIGH_EQ:
            return val1 >= val2;
        case COMP_TYPE_LOW_EQ:
            return val1 <= val2;
        default:
            // incorrect parameter
            ASSERT(false);
            return false;
    }
}

TC_COMMON_API bool IsIPAddress(char const* ipaddress);
TC_COMMON_API uint32 CreatePIDFile(const std::string& filename);


TC_COMMON_API std::string ByteArrayToHexStr(uint8 const* bytes, uint32 length, bool reverse = false);
TC_COMMON_API void HexStrToByteArray(std::string const& str, uint8* out, bool reverse = false);

TC_COMMON_API bool StringToBool(std::string const& str);

TC_COMMON_API bool StringContainsStringI(std::string const& haystack, std::string const& needle);
template <typename T>
inline bool ValueContainsStringI(std::pair<T, std::string> const& haystack, std::string const& needle)
{
    return StringContainsStringI(haystack.second, needle);
}

class TC_COMMON_API Tokenizer
{
public:
    typedef std::vector<char const*> StorageType;

    typedef StorageType::size_type size_type;

    typedef StorageType::const_iterator const_iterator;
    typedef StorageType::reference reference;
    typedef StorageType::const_reference const_reference;

public:
    Tokenizer(const std::string &src, char const sep, uint32 vectorReserve = 0);
    ~Tokenizer() { delete[] m_str; }

    const_iterator begin() const { return m_storage.begin(); }
    const_iterator end() const { return m_storage.end(); }

    size_type size() const { return m_storage.size(); }

    reference operator [] (size_type i) { return m_storage[i]; }
    const_reference operator [] (size_type i) const { return m_storage[i]; }

private:
    char* m_str;
    StorageType m_storage;
};

static float GetDistance(float x, float y, float x2, float y2) 
{
    float dx = x2 - x;
    float dy = y2 - y;
    float dist = sqrt((dx*dx) + (dy*dy));
    return ( dist > 0 ? dist : 0);     
};

//return true of timer reached 0
static bool DecreaseTimer(uint32& timer, uint32 const diff)
{
    if (timer > 0)
    {
        if (diff >= timer)
            timer = 0;
        else
        {
            timer -= diff;
            return false;
        }
    }
    return true;
};

template<typename E>
typename std::underlying_type<E>::type AsUnderlyingType(E enumValue)
{
    static_assert(std::is_enum<E>::value, "AsUnderlyingType can only be used with enums");
    return static_cast<typename std::underlying_type<E>::type>(enumValue);
}

/* Convert floating point chance to premultiplied integer chance (100.00 = 10000). */
inline uint32 chance_u(float chance)
{
    return uint32(::roundf(std::max(0.0f, chance) * 100)); // Nearest 2 decimal places
}

/* An abstract die for combat rolls with premultiplied integer chances */
template<class Side, Side Default, uint8 Sides>
struct Die
{
    explicit Die() {}
    Side roll(uint32 random)
    {
        uint32 rolling = 0;
        for (uint8 side = 0; side < Sides; ++side)
        {
            if (chance[side])
            {
                rolling += chance[side];
                if (random <= rolling)
                    return Side(side);
            }
        }
        return Default;
    }
    void set(uint8 side, float chancef)
    {
        if (side < Sides)
            chance[side] = chance_u(chancef);
    }
    uint32 chance[Sides] = { };
};

#endif

