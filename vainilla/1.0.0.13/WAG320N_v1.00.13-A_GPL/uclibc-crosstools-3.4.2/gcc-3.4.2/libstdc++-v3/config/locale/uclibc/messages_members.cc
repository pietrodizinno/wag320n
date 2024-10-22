// std::messages implementation details, GNU version -*- C++ -*-

// Copyright (C) 2001, 2002 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

//
// ISO C++ 14882: 22.2.7.1.2  messages virtual functions
//

// Written by Benjamin Kosnik <bkoz@redhat.com>

#include <locale>
#include <bits/c++locale_internal.h>

#ifdef __UCLIBC_MJN3_ONLY__
#warning fix gettext stuff
#endif
#ifdef __UCLIBC_HAS_GETTEXT_AWARENESS__
extern "C" char *__dcgettext(const char *domainname,
			     const char *msgid, int category);
#undef gettext
#define gettext(msgid) __dcgettext(NULL, msgid, LC_MESSAGES)
#else
#undef gettext
#define gettext(msgid) (msgid)
#endif

namespace std
{
  // Specializations.
  template<>
    string
    messages<char>::do_get(catalog, int, int, const string& __dfault) const
    {
#ifdef __UCLIBC_HAS_XLOCALE__
      __c_locale __old = __uselocale(_M_c_locale_messages);
      const char* __msg = const_cast<const char*>(gettext(__dfault.c_str()));
      __uselocale(__old);
      return string(__msg);
#elif defined __UCLIBC_HAS_LOCALE__
      char* __old = strdup(setlocale(LC_ALL, NULL));
      setlocale(LC_ALL, _M_name_messages);
      const char* __msg = gettext(__dfault.c_str());
      setlocale(LC_ALL, __old);
      free(__old);
      return string(__msg);
#else
      const char* __msg = gettext(__dfault.c_str());
      return string(__msg);
#endif
    }

#ifdef _GLIBCXX_USE_WCHAR_T
  template<>
    wstring
    messages<wchar_t>::do_get(catalog, int, int, const wstring& __dfault) const
    {
# ifdef __UCLIBC_HAS_XLOCALE__
      __c_locale __old = __uselocale(_M_c_locale_messages);
      char* __msg = gettext(_M_convert_to_char(__dfault));
      __uselocale(__old);
      return _M_convert_from_char(__msg);
# elif defined __UCLIBC_HAS_LOCALE__
      char* __old = strdup(setlocale(LC_ALL, NULL));
      setlocale(LC_ALL, _M_name_messages);
      char* __msg = gettext(_M_convert_to_char(__dfault));
      setlocale(LC_ALL, __old);
      free(__old);
      return _M_convert_from_char(__msg);
# else
      char* __msg = gettext(_M_convert_to_char(__dfault));
      return _M_convert_from_char(__msg);
# endif
    }
#endif
}
