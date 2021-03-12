# modify node.gyp
# Read in the file
import sys
import platform


platform_system = platform.system()


def put_libraries(libraries):
    print(libraries)
    with open('nodejs/node.gypi', 'r') as file:
        filedata = file.read()
        print("AAA")


        if not '-lrt' in filedata:
            print("python in text")

    

            # Replace the target string
            filedata = filedata.replace('\'-lrt\'', '\'-lrt\',\'libpython3.8.so\'')

            # Write the file out again
            with open('nodejs/node.gypi', 'w') as file:
                file.write(filedata)
            pass


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

