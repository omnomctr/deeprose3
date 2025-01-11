import sys

if __name__ == "__main__":
    stdlib = open("programs/stdlib.deeprose", "r")
    out = open(f"{sys.argv[1]}/stdlib.h", "w");
    
    out.write(
'''
#ifndef STDLIB_HEADER__
#define STDLIB_HEADER__

/* this code was generated using the script create_stdlib_header.py */

const char * const stdlib = {"''')
    
    while True:
        char = stdlib.read(1)

        if not char:
            break

        if char == '\n':
            out.write('\\n')
            continue;
        elif char == '\\' or char == '"' or char == "'":
            out.write('\\')
        
        out.write(char)

    out.write(
'''"};

#endif
''')
    stdlib.close()
    out.close()
