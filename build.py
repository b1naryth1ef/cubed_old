#!/usr/bin/env python

import os, sys, time, fnmatch

# Linked libraries
LIBRARIES = [
    "sqlite3",
    "lua",
    "dl",
    "protobuf",
    "pthread"
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

def get_protobuf_command():
    if not os.path.exists("src/include/gen"):
        os.mkdir("src/include/gen")

    return "protoc --cpp_out=src/include/gen/ --proto_path=proto/ %s" % ' '.join(
        find_source_files("proto", ext=[".proto"]))

def gen_build_command():
    base = ["g++ -w -std=c++11"]
    base.append(" ".join(find_source_files(".")))
    base.append(" ".join(find_source_files("src/include/gen", ext=[".cc"])))
    base.append(" ".join(["-I%s" % i for i in INCLUDES]))
    base.append(" ".join(["-l%s" % i for i in LIBRARIES]))
    base.append(" ".join(["-D%s" % i for i in FLAGS]))
    base.append("-L/usr/local/lib")
    base.append("-o cubed")
    return " ".join(base)

def build():
    start = time.time()
    i = os.system(get_protobuf_command())
    i = i or os.system(gen_build_command())

    return not bool(i), (time.time() - start)

def setup_rapidjson():
    if os.path.exists("/usr/include/rapidjson"):
        os.system("sudo rm -rf /usr/include/rapidjson")

    os.system("git clone https://github.com/miloyip/rapidjson.git")
    os.system("sudo cp -rf rapidjson/include/rapidjson /usr/include/rapidjson")
    os.system("rm -rf rapidjson")

def setup_flatbuffers():
    start = os.getcwd()
    os.system("git clone https://github.com/google/flatbuffers.git")
    os.chdir("flatbuffers/build/")
    os.system("cmake ..")
    os.system("make -j8")
    os.system("sudo make install")
    os.chdir(start)
    os.system("rm -rf flatbuffers")

def setup_liblua():
    start = os.getcwd()
    os.system("wget http://www.lua.org/ftp/lua-5.2.3.tar.gz")
    os.system("tar xzvf lua-5.2.3.tar.gz")
    os.chdir("lua-5.2.3")
    os.system("make linux -j8")
    os.system("sudo make install")
    os.chdir(start)
    os.system("rm -rf lua-5.2.3 lua-5.2.3.tar.gz")

def setup():
    setup_liblua()
    setup_rapidjson()
    setup_flatbuffers()

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print "Usage: ./build.py <build|run|setup>"
        sys.exit(1)

    if sys.argv[1] == "setup":
        print "Setting up enviroment..."
        setup()

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

        os.system("./cubed %s" % ' '.join(sys.argv[2:]))
