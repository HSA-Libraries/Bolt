using namespace std;
/******************************************************************************
 *  User Defined Data Types - vec2,4,8
 *****************************************************************************/
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


    
/******************************************************************************
 *  User Defined Binary Functions - DATA_TYPE plus Only for thrust usage
 *****************************************************************************/
#if (Bolt_Benchmark == 0)
    struct vec1plus
        {
                    __host__ __device__
            DATA_TYPE operator()(const DATA_TYPE &lhs, const DATA_TYPE &rhs) const
            {
            DATA_TYPE l_result;
            l_result = lhs + rhs;
            return l_result;
            };
        }; 
#endif
/******************************************************************************
 *  User Defined Binary Functions - vec2,4,8plus
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(vec2plus,
    struct vec2plus
    {
        vec2 operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        vec2 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        return l_result;
        };
    }; 
    );
    BOLT_FUNCTOR(vec4plus,
    struct vec4plus
    {
        vec4 operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        vec4 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        return l_result;
        };
    }; 
    );
    BOLT_FUNCTOR(vec8plus,
    struct vec8plus
    {
        vec8 operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        vec8 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        l_result.e = lhs.e+rhs.e;
        l_result.f = lhs.f+rhs.f;
        l_result.g = lhs.g+rhs.g;
        l_result.h = lhs.h+rhs.h;
        return l_result;
        };
    }; 
    );
#else
    struct vec2plus
    {
        __host__ __device__
        vec2 operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        vec2 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        return l_result;
        }
    }; 
    struct vec4plus
    {
        __host__ __device__
        vec4 operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        vec4 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        return l_result;
        };
    }; 
    struct vec8plus
    {
        __host__ __device__
        vec8 operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        vec8 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        l_result.e = lhs.e+rhs.e;
        l_result.f = lhs.f+rhs.f;
        l_result.g = lhs.g+rhs.g;
        l_result.h = lhs.h+rhs.h;
        return l_result;
        };
}; 
#endif

/******************************************************************************
 *  User Defined Unary Functions-  DATA_TYPE square for thrust usage
 *****************************************************************************/
#if (Bolt_Benchmark == 0)
    struct vec1square
    {
            __host__ __device__
        DATA_TYPE operator()(const DATA_TYPE &rhs) const
        {
        DATA_TYPE l_result;
        l_result = rhs * rhs;
        return l_result;
        };
    }; 
#endif
/******************************************************************************
 *  User Defined Unary Functions-  vec2,4,8square
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(vec2square,
    struct vec2square
    {
        vec2 operator()(const vec2 &rhs) const
        {
        vec2 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        return l_result;
        };
    }; 
    );
    BOLT_FUNCTOR(vec4square,
    struct vec4square
    {
        vec4 operator()(const vec4 &rhs) const
        {
        vec4 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        return l_result;
        };
    }; 
    );
    BOLT_FUNCTOR(vec8square,
    struct vec8square
    {
        vec8 operator()(const vec8 &rhs) const
        {
        vec8 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        l_result.e = rhs.e*rhs.e;
        l_result.f = rhs.f*rhs.f;
        l_result.g = rhs.g*rhs.g;
        l_result.h = rhs.h*rhs.h;
        return l_result;
        };
    }; 
    );
#else
    struct vec2square
    {
            __host__ __device__
        vec2 operator()(const vec2 &rhs) const
        {
        vec2 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        return l_result;
        };
    }; 
    struct vec4square
    {
            __host__ __device__
        vec4 operator()(const vec4 &rhs) const
        {
        vec4 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        return l_result;
        };
    }; 
    struct vec8square
    {
            __host__ __device__
        vec8 operator()(const vec8 &rhs) const
        {
        vec8 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        l_result.e = rhs.e*rhs.e;
        l_result.f = rhs.f*rhs.f;
        l_result.g = rhs.g*rhs.g;
        l_result.h = rhs.h*rhs.h;
        return l_result;
        };
    }; 
#endif
#if (Bolt_Benchmark == 0)
/******************************************************************************
 *  User Defined Binary Predicates-  DATA_TYPE equal for thrust usage
 *****************************************************************************/
    struct vec1equal
    {
            __host__ __device__
        bool operator()(const DATA_TYPE &lhs, const DATA_TYPE &rhs) const
        {
        return lhs == rhs;
        };
    }; 
#endif
/******************************************************************************
 *  User Defined Binary Predicates- vec2,4,8equal   
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(vec2equal,
    struct vec2equal
    {
        bool operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        return lhs == rhs;
        };
    }; 
    );
    BOLT_FUNCTOR(vec4equal,
    struct vec4equal
    {
        bool operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        return lhs == rhs;
        };
    }; 
    );
    BOLT_FUNCTOR(vec8equal,
    struct vec8equal
    {
        bool operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        return lhs == rhs;
        };
    }; 
    );
#else
    struct vec2equal
    {
            __host__ __device__
        bool operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        return lhs == rhs;
        };
        }; 
    struct vec4equal
    {
            __host__ __device__
        bool operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        return lhs == rhs;
        };
    }; 
    struct vec8equal
    {
            __host__ __device__
        bool operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        return lhs == rhs;
        };
    }; 
#endif
#if (Bolt_Benchmark == 0)
/******************************************************************************
 *  User Defined Binary Predicates DATA_TYPE less than for thrust usage
 *****************************************************************************/
    struct vec1less
    {
            __host__ __device__
        bool operator()(const DATA_TYPE &lhs, const DATA_TYPE &rhs) const
        {
        if (lhs < rhs) return true;
        };
    }; 
#endif
/******************************************************************************
 *  User Defined Binary Predicates- vec2,4,8 less than  
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(vec2less,
    struct vec2less
    {
        bool operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        return false;
        };
    }; 
    );
    BOLT_FUNCTOR(vec4less,
    struct vec4less
    {
        bool operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        return false;
        };
    }; 
    );
    BOLT_FUNCTOR(vec8less,
    struct vec8less
    {
        bool operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        if (lhs.e < rhs.e) return true;
        if (lhs.f < rhs.f) return true;
        if (lhs.g < rhs.g) return true;
        if (lhs.h < rhs.h) return true;
        return false;
        };
    }; 
    );
#else
    struct vec2less
    {
            __host__ __device__
        bool operator()(const vec2 &lhs, const vec2 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        return false;
        };
    }; 
    struct vec4less
    {
            __host__ __device__
        bool operator()(const vec4 &lhs, const vec4 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        return false;
        };
    }; 
    struct vec8less
    {
            __host__ __device__
        bool operator()(const vec8 &lhs, const vec8 &rhs) const
        {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        if (lhs.e < rhs.e) return true;
        if (lhs.f < rhs.f) return true;
        if (lhs.g < rhs.g) return true;
        if (lhs.h < rhs.h) return true;
        return false;
        };
    }; 
#endif
/******************************************************************************
 *  User Defined generator-  DATATYPE and vec2,4,8
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
    BOLT_FUNCTOR(intgen,
    struct intgen
    {
        DATA_TYPE operator()() const
        {
        DATA_TYPE v = 1;
        return v;
        };
    }; 
    );
    BOLT_FUNCTOR(vec2gen,
    struct vec2gen
    {
        vec2 operator()() const
        {
        vec2 v = { 2, 3 };
        return v;
        };
    }; 
    );
    BOLT_FUNCTOR(vec4gen,
    struct vec4gen
    {
        vec4 operator()() const
        {
        vec4 v = { 4, 5, 6, 7 };
        return v;
        };
    }; 
    );
    BOLT_FUNCTOR(vec8gen,
    struct vec8gen
    {
        vec8 operator()() const
        {
        vec8 v = { 8, 9, 10, 11, 12, 13, 14, 15 };
        return v;
        };
    }; 
    );
#else
    
    struct intgen
    {
            __host__ __device__
        DATA_TYPE operator()() const
        {
        DATA_TYPE v = 1;
        return v;
        };
    }; 
    struct vec2gen
    {
            __host__ __device__
        vec2 operator()() const
        {
        vec2 v = { 2, 3 };
        return v;
        };
    }; 
    struct vec4gen
    {
            __host__ __device__
        vec4 operator()() const
        {
        vec4 v = { 4, 5, 6, 7 };
        return v;
        };
    }; 
    struct vec8gen
    {
            __host__ __device__
        vec8 operator()() const
        {
        vec8 v = { 8, 9, 10, 11, 12, 13, 14, 15 };
        return v;
        };
    }; 
#endif

    int ValidateBenchmarkKey(const char *par_instr,std::string keys [],int len)
    {
    int loc_mid,
        loc_high,
        loc_low;

    loc_low = 0;
    loc_high = len-1;
    /*
     *Binary search.
     */
    while(loc_low <= loc_high)
    {
        loc_mid =((loc_low + loc_high) / 2);

        if(strcmp((const char  *)keys[loc_mid].c_str(),(const char  *)par_instr) < 0 )
        {
            loc_low = (loc_mid + 1);
        }
        else if (strcmp((const char  *)keys[loc_mid].c_str(),(const char  *)par_instr) > 0 )
        {
            loc_high = (loc_mid - 1);
        }
        else
        { 
            return loc_mid;									

        }
    }

       /* Not a key word */
    return -1;
}