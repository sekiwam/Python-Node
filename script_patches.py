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
            filedata = filedata.replace('\'-lrt\'', '\'-lrt\'], \'libraries\': [\'libpython3.8.so\'')

            # Write the file out again
            with open('nodejs/node.gypi', 'w') as file:
                file.write(filedata)

    with open('nodejs/node.gyp', 'r') as file:
        filedata = file.read()


# 'ldflags': [ "-Wl,-rpath='$$ORIGIN/./'"],
# 'target_name': 'node_mksnapshot'
        if not '/./' in filedata:
            # Replace the target string
            filedata = filedata.replace("'target_name': 'node_mksnapshot'", "'target_name': 'node_mksnapshot', 'ldflags': [ \"-Wl,-rpath='$$ORIGIN/./'\"]")

            # Write the file out again
            with open('nodejs/node.gyp', 'w') as file:
                file.write(filedata)

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

