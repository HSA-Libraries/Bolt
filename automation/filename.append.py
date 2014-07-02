############################################################################
#   (c) 2012,2014 Advanced Micro Devices, Inc. All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
############################################################################
#!/usr/bin/env python
from __future__ import print_function

import sys, os
from glob import glob
from sys import platform as _platform

globMask        = sys.argv[1]
configuration   = sys.argv[2]

for oldName in glob( globMask ):
    filename = os.path.basename( oldName )
    basename, extention1 = os.path.splitext( filename )
    extention2 = ""
    if _platform != "win32":
        basename, extention2 = os.path.splitext( basename )
 
    # 3rd parameter and beyond begins list of strings that we want to skip
    # iterate through and skip file if we have already touched the file once 
    found = False
    for exclude in sys.argv[3:]:
        if exclude in basename:
            found = True
            break
    
        if found == True:
            continue
    
    newName = basename + configuration + extention2 + extention1
    print( oldName, " -> ", newName )
    os.rename( oldName, newName )
