// Wrapper for underlying C-language localization -*- C++ -*-

// Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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
// ISO C++ 14882: 22.8  Standard locale categories.
//

// Written by Benjamin Kosnik <bkoz@redhat.com>

#include <cerrno>  // For errno
#include <locale>
#include <stdexcept>
#include <langinfo.h>
#include <bits/c++locale_internal.h>

#ifndef __UCLIBC_HAS_XLOCALE__
#define __strtol_l(S, E, B, L)      strtol((S), (E), (B))
#define __strtoul_l(S, E, B, L)     strtoul((S), (E), (B))
#define __strtoll_l(S, E, B, L)     strtoll((S), (E), (B))
#define __strtoull_l(S, E, B, L)    strtoull((S), (E), (B))
#define __strtof_l(S, E, L)         strtof((S), (E))
#define __strtod_l(S, E, L)         strtod((S), (E))
#define __strtold_l(S, E, L)        strtold((S), (E))
#warning should dummy __newlocale check for C|POSIX ?
#define __newlocale(a, b, c)        NULL
#define __freelocale(a)             ((void)0)
#define __duplocale(a)              __c_locale()
#endif

namespace std 
{
  template<>
    void
    __convert_to_v(const char* __s, float& __v, ios_base::iostate& __err, 
		   const __c_locale& __cloc)
    {
      if (!(__err & ios_base::failbit))
	{
	  char* __sanity;
	  errno = 0;
	  float __f = __strtof_l(__s, &__sanity, __cloc);
          if (__sanity != __s && errno != ERANGE)
	    __v = __f;
	  else
	    __err |= ios_base::failbit;
	}
    }

  template<>
    void
    __convert_to_v(const char* __s, double& __v, ios_base::iostate& __err, 
		   const __c_locale& __cloc)
    {
      if (!(__err & ios_base::failbit))
	{
	  char* __sanity;
	  errno = 0;
	  double __d = __strtod_l(__s, &__sanity, __cloc);
          if (__sanity != __s && errno != ERANGE)
	    __v = __d;
	  else
	    __err |= ios_base::failbit;
	}
    }

  template<>
    void
    __convert_to_v(const char* __s, long double& __v, ios_base::iostate& __err,
		   const __c_locale& __cloc)
    {
      if (!(__err & ios_base::failbit))
	{
	  char* __sanity;
	  errno = 0;
	  long double __ld = __strtold_l(__s, &__sanity, __cloc);
          if (__sanity != __s && errno != ERANGE)
	    __v = __ld;
	  else
	    __err |= ios_base::failbit;
	}
    }

  void
  locale::facet::_S_create_c_locale(__c_locale& __cloc, const char* __s, 
				    __c_locale __old)
  {
    __cloc = __newlocale(1 << LC_ALL, __s, __old);
#ifdef __UCLIBC_HAS_XLOCALE__
    if (!__cloc)
      {
	// This named locale is not supported by the underlying OS.
	__throw_runtime_error(__N("locale::facet::_S_create_c_locale "
			      "name not valid"));
      }
#endif
  }
  
  void
  locale::facet::_S_destroy_c_locale(__c_locale& __cloc)
  {
    if (_S_get_c_locale() != __cloc)
      __freelocale(__cloc); 
  }

  __c_locale
  locale::facet::_S_clone_c_locale(__c_locale& __cloc)
  { return __duplocale(__cloc); }
} // namespace std

namespace __gnu_cxx
{
  const char* const category_names[6 + _GLIBCXX_NUM_CATEGORIES] =
    {
      "LC_CTYPE", 
      "LC_NUMERIC",
      "LC_TIME", 
      "LC_COLLATE", 
      "LC_MONETARY",
      "LC_MESSAGES", 
#if _GLIBCXX_NUM_CATEGORIES != 0
      "LC_PAPER", 
      "LC_NAME", 
      "LC_ADDRESS",
      "LC_TELEPHONE", 
      "LC_MEASUREMENT", 
      "LC_IDENTIFICATION" 
#endif
    };
}

namespace std
{
  const char* const* const locale::_S_categories = __gnu_cxx::category_names;
}  // namespace std
