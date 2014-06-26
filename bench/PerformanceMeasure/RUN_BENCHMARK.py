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
'''
-lib bolt -> to run the Bolt Benchmark 
-lib thrust -> to run the thrust Benchmark 
'''
import os
os.system("python benchmark.py -r reduce -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r reducebykey -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r transformreduce -l 4096-33554432:x2 -i 1000 -lib bolt")

os.system("python benchmark.py -r sort -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r sortbykey -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r stablesort -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r stablesortbykey -l 4096-33554432:x2 -i 1000 -lib bolt")

os.system("python benchmark.py -r scan -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r scanbykey -l 4096-33554432:x2 -i 1000 -lib bolt")
os.system("python benchmark.py -r transformscan -l 4096-33554432:x2 -i 1000 -lib bolt")

os.system("python benchmark.py -r unarytransform -l 4096-33554432:x2 -i 1000 -lib bolt")
