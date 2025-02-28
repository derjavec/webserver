#!/bin/bash

echo "Content-Type: text/html"
echo ""

METHOD=${REQUEST_METHOD:-GET}

if [ "$METHOD" = "POST" ]; then
    read -n "$CONTENT_LENGTH" POST_DATA
    NAME="Guest"
    
    if [[ "$POST_DATA" =~ name=(.*) ]]; then
        NAME="${BASH_REMATCH[1]}"
    fi
    
    echo "<h1>Welcome, $NAME!</h1>"
else
    echo "<h1>Hello! This is a static message from SHELL.</h1>"
fi
