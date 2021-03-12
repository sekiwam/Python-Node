# modify node.gyp
# Read in the file
import sys

print("args = "  + str(sys.argv))

# validates args
if len(sys.argv) < 1:
    exit(32341)

arg0 = sys.argv[0]
print(arg0)





with open('nodejs/node.gyp', 'r') as file:
  filedata = file.read()

if 'python' in filedata:
    print("python in text")
    exit


# Replace the target string
filedata = filedata.replace('ram', 'abcd')

# Write the file out again
with open('nodejs/node_test.gyp', 'w') as file:
  file.write(filedata)