import os
print("iarbuild uPy_iar.ewp Debug > output.txt")
os.system("iarbuild uPy_iar.ewp Debug > output.txt")

with open("output.txt", "r") as f:
    contents = f.read()

fw_lines = list(filter(lambda x: "Warning[" in x, contents.splitlines()))
f_lines = list(filter(lambda x: "Error[" in x, contents.splitlines()))

print("\n".join(f_lines))
print(f"{len(f_lines)} errors")
print(f"{len(fw_lines)} warnings")