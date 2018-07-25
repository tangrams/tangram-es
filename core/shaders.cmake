file(READ "${in}" file_content)
# drop single line comments
string(REGEX REPLACE "([\n][ \t]*(//)[^\n]*)" "" content "${file_content}")
# drop empty lines
string(REGEX REPLACE "[\n]+" "\n" content "${content}")
file(WRITE "${out}" "const static char* ${name} = R\"RAW_GLSL(\n${content})RAW_GLSL\";\n")
