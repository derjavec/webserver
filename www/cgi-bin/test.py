#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/html\r\n\r\n")

if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    post_data = sys.stdin.read(content_length)
    name = post_data.split('=')[-1] if 'name=' in post_data else 'Guest'
    print(f"<h1>Welcome, {name}!</h1>")
else:
    print("<h1>Hello! This is a static message from Python.</h1>")
