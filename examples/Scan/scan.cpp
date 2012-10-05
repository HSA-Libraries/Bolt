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

#include "stdafx.h"

#include <bolt/cl/scan.h>

#include <vector>
#include <numeric>

int _tmain( int argc, _TCHAR* argv[ ] )
{
    bolt::cl::device_vector< int > boltInput( 1024ul, 1 );
    bolt::cl::device_vector< int >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    std::vector< int > stdInput( 1024ul, 1 );
    std::vector< int >::iterator stdEnd  = bolt::cl::inclusive_scan( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    
	return 0;
}