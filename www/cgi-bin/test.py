#!/usr/bin/python3
import sys
try:
    print("Content-Type: text/html\n")
    for i in range(100):
        print(f"Hello from test CGI, line {i}")
    sys.stdout.flush()
except BrokenPipeError:
    sys.exit(0)
