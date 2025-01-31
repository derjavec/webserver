#!/usr/bin/env python3

import sys
import os
import cgi

# 📌 Leer datos del formulario
form = cgi.FieldStorage()

# 📌 Obtener valores del formulario
name = form.getvalue("name", "Unknown")  # Si no hay valor, usa "Unknown"
message = form.getvalue("message", "")

# 📌 Generar respuesta HTTP con HTML
print("Content-Type: text/html\r\n")
print("<html><head><title>Contact Form</title></head><body>")
print(f"<h1>Thank You, {name}!</h1>")
print(f"<p>Your message: {message}</p>")
print("</body></html>")
