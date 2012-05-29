#ifndef UNVC_H
#define UNVC_H

// hiding visual c++ features from gcc
#define AM_NOVTABLE
#define __format_string
#define __in
#define __out
#define __inout
#define __in_opt
#define __out_opt
#define __inout_opt
#define __deref_out
#define __deref_out_opt
#define __deref_in
#define __deref_inout_opt
#define __in_bcount(x)
#define __in_ecount(x)
#define __in_bcount_opt(x)
#define __in_ecount_opt(x)
#define __out_bcount(x)
#define __out_ecount(x)
#define __out_bcount_opt(x)
#define __out_ecount_opt(x)
#define __out_bcount_part(x,y)
#define __out_ecount_part(x,y)
#define __field_ecount_opt(x)
#define __range(x,y)
#define __inout_ecount_full(x)
#define EXTERN_GUID(g,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8) DEFINE_GUID(g,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)

#endif