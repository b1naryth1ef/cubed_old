#!/usr/bin/env python

import os, sys, time, fnmatch

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

def gen_flatc_command():
    return "flatc -o src/include/gen/ -c %s" % ' '.join(find_source_files("proto", ext=[".idl"]))


def gen_build_command():
    base = ["g++ -w -std=c++11 -o cubed.o"]
    base.append(" ".join(find_source_files(".")))
    base.append(" ".join(["-I%s" % i for i in INCLUDES]))
    base.append(" ".join(["-l%s" % i for i in LIBRARIES]))
    base.append(" ".join(["-D%s" % i for i in FLAGS]))
    base.append("-L/usr/local/lib")
    return " ".join(base)

def build():
    start = time.time()
    i = os.system(gen_flatc_command())
    i = i or os.system(gen_build_command())

    return not bool(i), (time.time() - start)

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print "Usage: ./build.py <build|run>"
        sys.exit(1)

    if sys.argv[1] == "build":
        print "Building..."
        success, dura = build()
        if success:
            print "FINISHED! Took %ss" % dura

    if sys.argv[1] == "run":
        print "Building and running..."
        success, _ = build()
        if not success:
            print "Errors during build, not running..."
            sys.exit(1)

        os.system("./cubed.o")
