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

#pragma once


namespace bolt {
	namespace cl {

		typedef int ptrdiff_t; // Map all pointer differences to 32-bit integers for OCL benefit.

		template <class Iterator> struct iterator_traits {
			typedef typename Iterator::difference_type difference_type;
			//typedef int difference_type;  // FIXME, this should call down to the lower-level iterators.
			typedef typename Iterator::value_type value_type;
			typedef typename Iterator::pointer pointer;
			typedef typename Iterator::reference reference;
			typedef typename Iterator::iterator_category iterator_category;
		};


		template <class T> struct iterator_traits<T*> {
			typedef ptrdiff_t difference_type;
			typedef T value_type;
			typedef T* pointer;
			typedef T& reference;
			typedef std::random_access_iterator_tag iterator_category;
		};

		template <class T> struct iterator_traits<const T*> {
			typedef ptrdiff_t difference_type;
			typedef T value_type;
			typedef const T* pointer;
			typedef const T& reference;
			typedef std::random_access_iterator_tag iterator_category;
		};


	}
};
