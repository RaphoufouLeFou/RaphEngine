# This script creates a header file with the shader code as strings

import os

shader_folder = "../shader/"
output_file = "../RaphEngine/shader/shaders.h"
outputCfile = "../RaphEngine/shader/shaders.cpp"

# Clear the file
file = open(output_file, "w")
file.truncate(0)
file.write("#pragma once\n\n")
file.close()

file = open(outputCfile, "w")
file.truncate(0)

file.write("#include \"shaders.h\"\n")
file.write("#include \"pch.h\"\n\n")
file.close()

for shader in os.listdir(shader_folder):
    if not shader.endswith(".glsl"):
        continue
    with open(shader_folder + shader, "r") as file:
        shader_code = file.read()
        shader_name = shader.replace(".glsl", "").replace(" ", "_").replace(".", "_").replace("-", "_") + "_shader"
        with open(outputCfile, "a") as output:
            output.write("const char* " + shader_name + " = R\"(\n" + shader_code + "\n)\";\n\n")
    
    with open(output_file, "a") as output:
        output.write("extern const char* " + shader_name + ";\n")