#!/usr/bin/python3

import sys
import os
import urllib.parse

print("Content-Type: text/html\n")

content_length = int(os.environ.get("CONTENT_LENGTH", 0)) 
post_data = sys.stdin.read(content_length) if content_length > 0 else ""

form_data = urllib.parse.parse_qs(post_data)

name = form_data.get("name", ["Unknown"])[0]
message = form_data.get("message", ["No message provided"])[0]

print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Contact Response</title>
</head>
<body>
    <h1>Message Received</h1>
    <p><strong>Name:</strong> {name}</p>
    <p><strong>Message:</strong> {message}</p>
    <a href="/contact.html">Go back</a>
</body>
</html>
""")



