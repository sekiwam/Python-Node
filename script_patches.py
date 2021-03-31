# modify node.gyp
# Read in the file
import sys
import platform

platform_system = platform.system()


def put_libraries(libraries):
    print(libraries)
    with open('nodejs/node.gypi', 'r') as file:
        filedata = file.read()

        if not 'libpython3' in filedata:
            # Replace the target string
            filedata = filedata.replace('\'-lrt\'', "\'-lrt\'], 'ldflags': [ \"-Wl,-rpath='$$ORIGIN/./'\"], \'libraries\': [\'libpython3.8.so\'")

            # Write the file out again
            with open('nodejs/node.gypi', 'w') as file:
                file.write(filedata)


    # with open('nodejs/node.gyp', 'r') as file:
    #     filedata = file.read()

    #     # 'ldflags': [ "-Wl,-rpath='$$ORIGIN/./'"],

    #     if not '/./' in filedata:
    #         # Replace the target string
    #         filedata = filedata.replace("'target_name': 'node_mksnapshot'", "'target_name': 'node_mksnapshot', 'ldflags': [ \"-Wl,-rpath='$$ORIGIN/./'\"]")
            
    #         filedata = filedata.replace("'target_name': 'mkcodecache'", "'target_name': 'mkcodecache', 'ldflags': [ \"-Wl,-rpath='$$ORIGIN/./'\"]")

    #         # Replace the target string
    #         filedata = filedata.replace("'target_name': 'cctest'", "'target_name': 'cctest', 'ldflags': [ \"-Wl,-rpath='$$ORIGIN/./'\"]")

    #         # Write the file out again
    #         with open('nodejs/node.gyp', 'w') as file:
    #             file.write(filedata)

print("args = " + str(sys.argv))

# validates args
if len(sys.argv) < 1:
    exit(32341)


arg0 = sys.argv[0]
print(arg0)

libraries = None
if platform_system == "Windows":
    pass
elif platform_system == "Linux":
    libraries = "libpython3.8.so"
    pass
elif platform_system == "Darwin":
    pass

if libraries:
    put_libraries(libraries)





embed_path_a = "EMBED_SCRIPT_A.py"
embed_path_b = "EMBED_SCRIPT_B.py"
target_path = "nodejs/src/python_bind.cc"
#
#[EMBED_PYTHON_SCRIPT_B]
#[EMBED_PYTHON_SCRIPT_A]
#
with open(embed_path_a) as embed_file_a:
    embed_str_a = embed_file_a.read()

with open(embed_path_b) as embed_file_b:
    embed_str_b = embed_file_b.read()

with open(target_path, encoding='utf-8') as target_file:
    target_str = target_file.read()
    print("t")
    print(len(target_str))
    print("g")

    dst = target_str.replace('#[EMBED_PYTHON_SCRIPT_B]', embed_str_b)
    dst = dst.replace('#[EMBED_PYTHON_SCRIPT_A]', embed_str_a)

if dst:
    with open(target_path, mode='w', encoding='utf-8') as target_file:
        target_file.write(dst)