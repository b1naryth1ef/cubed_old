#!/usr/bin/env python

import os, fnmatch

# Linked libraries
LIBRARIES = [
    "sqlite3"
]

# Included directories or files
INCLUDES = [
    "src/include/",
]

# Source files to ignore
FIGNORES = []

# Flags
FLAGS = []

def find_source_files(start_dir, ext=[".cpp", ".c"]):
    result = []
    for (path, folders, files) in os.walk(start_dir):
        for fname in files:
            if any(map(lambda i: fnmatch.fnmatch(fname, i), FIGNORES)):
                continue
            if any(map(lambda i: fname.endswith(i), ext)):
                result.append(os.path.join(path, fname))
    return result

def gen_build_command():
    base = ["g++ -w -std=c++11 -o cubed.o"]
    base.append(" ".join(find_source_files(".")))
    base.append(" ".join(["-I%s" % i for i in INCLUDES]))
    base.append(" ".join(["-l%s" % i for i in LIBRARIES]))
    base.append(" ".join(["-D%s" % i for i in FLAGS]))
    base.append("-L/usr/local/lib")
    return " ".join(base)

if __name__ == "__main__":
    print "Building..."
    os.system(gen_build_command())
