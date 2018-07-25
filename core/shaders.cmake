file(READ  "${in}" content)
file(WRITE "${out}" "const static char* ${name} = R\"( ${content} )\";")
