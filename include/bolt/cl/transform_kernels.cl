/***************************************************************************
*   Copyright 2012 Advanced Micro Devices, Inc.
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.

***************************************************************************/

template< typename iNakedType1, typename iIterType1, typename iNakedType2, typename iIterType2, typename oNakedType, 
    typename oIterType, typename binary_function >
kernel
void transformTemplate (
            global iNakedType1* A_ptr,
            iIterType1 A_iter,
            global iNakedType2* B_ptr,
            iIterType2 B_iter,
            global oNakedType* Z_ptr,
            oIterType Z_iter,
            const uint length,
            global binary_function* userFunctor )
{
    int gx = get_global_id( 0 );
    if (gx >= length)
        return;

    A_iter.init( A_ptr );
    B_iter.init( B_ptr );
    Z_iter.init( Z_ptr );
    
    iNakedType1 aa = A_iter[ gx ];
    iNakedType2 bb = B_iter[ gx ];
    
    Z_iter[ gx ] = (*userFunctor)( aa, bb );
}

template< typename iNakedType1, typename iIterType1, typename iNakedType2, typename iIterType2, typename oNakedType, 
    typename oIterType, typename binary_function >
kernel
void transformNoBoundsCheckTemplate (
            global iNakedType1* A_ptr,
            iIterType1 A_iter,
            global iNakedType2* B_ptr,
            iIterType2 B_iter,
            global oNakedType* Z_ptr,
            oIterType Z_iter,
            const uint length,
            global binary_function* userFunctor)
{
    int gx = get_global_id( 0 );
    A_iter.init( A_ptr );
    B_iter.init( B_ptr );
    Z_iter.init( Z_ptr );

    iNakedType1 aa = A_iter[ gx ];
    iNakedType2 bb = B_iter[ gx ];
    
    Z_iter[ gx ] = (*userFunctor)( aa, bb );
}

template <typename iNakedType, typename iIterType, typename oNakedType, typename oIterType, typename unary_function >
kernel
void unaryTransformTemplate(
            global iNakedType* A_ptr,
            iIterType A_iter,
            global oNakedType* Z_ptr,
            oIterType Z_iter,
            const uint length,
            global unary_function* userFunctor)
{
    int gx = get_global_id( 0 );
    if (gx >= length)
        return;

    A_iter.init( A_ptr );
    Z_iter.init( Z_ptr );

    iNakedType aa = A_iter[ gx ];
    Z_iter[ gx ] = (*userFunctor)( aa );
}

template <typename iNakedType, typename iIterType, typename oNakedType, typename oIterType, typename unary_function >
kernel
void unaryTransformNoBoundsCheckTemplate(
            global iNakedType* A_ptr,
            iIterType A_iter,
            global oNakedType* Z_ptr,
            oIterType Z_iter,
            const uint length,
            global unary_function* userFunctor)
{
    int gx = get_global_id( 0 );

    A_iter.init( A_ptr );
    Z_iter.init( Z_ptr );

    iNakedType aa = A_iter[ gx ];
    Z_iter[ gx ] = (*userFunctor)( aa );
}