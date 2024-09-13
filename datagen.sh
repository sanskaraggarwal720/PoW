#!/bin/bash

# File to output the simulated data
output_file="ddos_data.csv"

# List of possible IP addresses
ip_addresses=("127.0.0.1" "192.168.0.1" "10.0.0.1" "172.16.0.1" "203.0.113.0" "198.51.100.0")

# List of possible browsers
browsers=("NOT-FOUND" "Safari" "Chrome" "Firefox" "Edge" "Opera")

# List of possible HTTP methods
http_methods=("GET" "POST" "HEAD" "OPTIONS")

# List of possible status codes
status_codes=("200" "301" "400" "403" "404" "500" "502")

# Function to generate a random timestamp using a simpler format
random_timestamp() {
    echo "$(date '+%s%3N')"
}

# Function to generate a random request count
random_request_count() {
    echo $((RANDOM % 100 + 1))
}

# Function to generate a random response time in milliseconds
# Adjusts response time based on whether it's a DDoS attempt or not
random_response_time() {
    local is_ddos=$1
    if [ "$is_ddos" = "true" ]; then
        echo $((RANDOM % 2000 + 800))  # Response time between 1000 and 3000 ms
    else
        echo $((RANDOM % 1000 + 200))    # Response time between 200 and 1000 ms
    fi
}

# Function to pick a random element from an array
pick_random() {
    local array=("$@")
    echo "${array[RANDOM % ${#array[@]}]}"
}

# Function to generate DDoS attempt data
generate_ddos_attempt() {
    local ddos_ip=$1
    local ddos_browser=$2
    local ddos_http_method=$3
    local ddos_status_code=$4

    for j in {1..1000}; do
        timestamp=$(random_timestamp)
        request_count=$((RANDOM % 300 + 200))  # Request count between 200 and 500
        response_time=$(random_response_time true)

        echo "$timestamp,$ddos_ip,$ddos_browser,$request_count,$ddos_http_method,$ddos_status_code,$response_time,DDOS" >> "$output_file"
    done
}

# Generate the file header
echo "Timestamp,IP Address,Browser,Request Count,HTTP Method,Status Code,Response Time (ms),DDoS Happening" > "$output_file"

# Generate 999,000 normal entries
for ((i=1; i<=999000; i++)); do
    echo $i
    timestamp=$(random_timestamp)
    ip=$(pick_random "${ip_addresses[@]}")
    browser=$(pick_random "${browsers[@]}")
    request_count=$(random_request_count)
    http_method=$(pick_random "${http_methods[@]}")
    status_code=$(pick_random "${status_codes[@]}")
    response_time=$(random_response_time false)

    echo "$timestamp,$ip,$browser,$request_count,$http_method,$status_code,$response_time,NoDDoS" >> "$output_file"

    if (( i % 10000 == 0 )); then
        echo "DDOS attempt at $i"
        generate_ddos_attempt "203.0.113.0" "Chrome" "GET" "200"
        generate_ddos_attempt "198.51.100.0" "Firefox" "POST" "502"
    fi
done

echo "Generated $output_file with 1 million entries including simulated DDoS attempts."
