#!/usr/bin/env python
from __future__ import print_function

import sys, os
from glob import glob

globMask        = sys.argv[1]
configuration   = sys.argv[2]

for oldName in glob( globMask ):
    filename = os.path.basename( oldName )
    basename, extention = os.path.splitext( filename )
    # 3rd parameter and beyond begins list of strings that we want to skip
    # iterate through and skip file if we have already touched the file once 
    found = False
    for exclude in sys.argv[3:]:
        if exclude in basename:
            found = True
            break
    
    if found == True:
        continue
    
    newName = basename + configuration + extention
    print( oldName, " -> ", newName )
    os.rename( oldName, newName )