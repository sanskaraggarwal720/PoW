#!/bin/bash

# Define the URL you want to send the request to
URL="http://example.com"

# Define the custom headers
HEADER_USER_AGENT="User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.102 Safari/537.36"
HEADER_ACCEPT="Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"
HEADER_CONNECTION="Connection: keep-alive"
HEADER_LANGUAGE="Accept-Language: en-US,en;q=0.5"

# Randomly decide whether to send headers (0 or 1)
SEND_HEADERS=$((RANDOM % 2))

# Send the GET request
if [ $SEND_HEADERS -eq 1 ]; then
    echo "Sending request with headers..."
    curl -X GET "$URL" \
        -H "$HEADER_USER_AGENT" \
        -H "$HEADER_ACCEPT" \
        -H "$HEADER_CONNECTION" \
        -H "$HEADER_LANGUAGE"
else
    echo "Sending request without headers..."
    curl -X GET "$URL"
fi
