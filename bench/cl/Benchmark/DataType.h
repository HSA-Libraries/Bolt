#include<iostream>

#include "bolt/cl/functional.h"
#include <bolt/unicode.h>
#include "bolt/unicode.h"
#include <CL/cl.h>
/******************************************************************************
 *  Data Types Enumerated
 *****************************************************************************/
enum dataType {
    t_int,
    t_vec2,
    t_vec4,
    t_vec8
};
static char *dataTypeNames[] = {
    "int1",
    "vec2",
    "vec4",
    "vec8"
};

#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(vec2,
    struct vec2
    {
        DATA_TYPE a, b;
        vec2  operator =(const DATA_TYPE inp)
        {
        vec2 tmp;
        a = b = tmp.a = tmp.b = inp;
        return tmp;
        }
        bool operator==(const vec2& rhs) const
        {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        return l_equal;
        }
      friend ostream& operator<<(ostream& os, const vec2& dt);
    };
    );
    BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec2 >::iterator );
    BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec2 >::iterator, bolt::cl::deviceVectorIteratorTemplate );
    BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, DATA_TYPE, vec2 );

      ostream& operator<<(ostream& os, const vec2& dt)
        {
        os<<dt.a<<" "<<dt.b;
        return os;
        }

    BOLT_FUNCTOR(vec4,
    struct vec4
    {
        DATA_TYPE a, b, c, d;
        vec4  operator =(const DATA_TYPE inp)
        {
        vec4 tmp;
        tmp.a = tmp.b = tmp.c = tmp.d = a = b = c=d=inp;
        return tmp;
        }
        bool operator==(const vec4& rhs) const
        {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        l_equal = ( c == rhs.c ) ? l_equal : false;
        l_equal = ( d == rhs.d ) ? l_equal : false;
        return l_equal;
        }
        friend ostream& operator<<(ostream& os, const vec4& dt);
    };
    );

        ostream& operator<<(ostream& os, const vec4& dt)
        {
        os<<dt.a<<" "<<dt.b<<" "<<dt.c<<" "<<dt.d;
        return os;
        }
    BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec4 >::iterator );
    BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec4 >::iterator, bolt::cl::deviceVectorIteratorTemplate );
    BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, DATA_TYPE, vec4 );

    BOLT_FUNCTOR(vec8,
    struct vec8
    {
        DATA_TYPE a, b, c, d, e, f, g, h;
        vec8  operator =(const DATA_TYPE inp)
        {
        a = b = c=d=e=f=g=h=inp;
        vec8 tmp;
        tmp.a = tmp.b = tmp.c = tmp.d = a = b = c=d=e=f=g=h=inp;
        tmp.e = tmp.f = tmp.g = tmp.h = inp;
        return tmp;
        }
        bool operator==(const vec8& rhs) const
        {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        l_equal = ( c == rhs.c ) ? l_equal : false;
        l_equal = ( d == rhs.d ) ? l_equal : false;
        l_equal = ( e == rhs.e ) ? l_equal : false;
        l_equal = ( f == rhs.f ) ? l_equal : false;
        l_equal = ( g == rhs.g ) ? l_equal : false;
        l_equal = ( h == rhs.h ) ? l_equal : false;
        return l_equal;
        }
        friend ostream& operator<<(ostream& os, const vec8& dt);

    };
    );
        ostream& operator<<(ostream& os, const vec8& dt)
        {
        os<<dt.a<<" "<<dt.b<<" "<<dt.c<<" "<<dt.d<<" "<<dt.e<<" "<<dt.f<<" "<<dt.g<<" "<<dt.h;
        return os;
        }

        BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec8 >::iterator );
        BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec8 >::iterator, bolt::cl::deviceVectorIteratorTemplate );
        BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, DATA_TYPE, vec8 );
        BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less, DATA_TYPE, vec8 );
#else
    struct vec2
        {
            DATA_TYPE a, b;
            __host__ __device__
            vec2  operator =(const DATA_TYPE inp)
            {
            vec2 tmp;
            a = b = tmp.a = tmp.b = inp;
            return tmp;
            }
            bool operator==(const vec2& rhs) const
            {
            bool l_equal = true;
            l_equal = ( a == rhs.a ) ? l_equal : false;
            l_equal = ( b == rhs.b ) ? l_equal : false;
            return l_equal;
            }
        };
    struct vec4
        {
            DATA_TYPE a, b, c, d;
            __host__ __device__
            vec4  operator =(const DATA_TYPE inp)
            {
            vec4 tmp;
            tmp.a = tmp.b = tmp.c = tmp.d = a = b = c=d=inp;
            return tmp;
            }
            bool operator==(const vec4& rhs) const
            {
            bool l_equal = true;
            l_equal = ( a == rhs.a ) ? l_equal : false;
            l_equal = ( b == rhs.b ) ? l_equal : false;
            l_equal = ( c == rhs.c ) ? l_equal : false;
            l_equal = ( d == rhs.d ) ? l_equal : false;
            return l_equal;
            }
        };
    struct vec8
        {
            DATA_TYPE a, b, c, d, e, f, g, h;
            __host__ __device__
            vec8  operator =(const DATA_TYPE inp)
            {
            a = b = c=d=e=f=g=h=inp;
            vec8 tmp;
            tmp.a = tmp.b = tmp.c = tmp.d = a = b = c=d=e=f=g=h=inp;
            tmp.e = tmp.f = tmp.g = tmp.h = inp;
            return tmp;
            }
            bool operator==(const vec8& rhs) const
            {
            bool l_equal = true;
            l_equal = ( a == rhs.a ) ? l_equal : false;
            l_equal = ( b == rhs.b ) ? l_equal : false;
            l_equal = ( c == rhs.c ) ? l_equal : false;
            l_equal = ( d == rhs.d ) ? l_equal : false;
            l_equal = ( e == rhs.e ) ? l_equal : false;
            l_equal = ( f == rhs.f ) ? l_equal : false;
            l_equal = ( g == rhs.g ) ? l_equal : false;
            l_equal = ( h == rhs.h ) ? l_equal : false;
            return l_equal;
            }
        };
#endif