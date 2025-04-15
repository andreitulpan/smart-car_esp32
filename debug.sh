#!/bin/bash

HOST="192.168.1.108"
PORT=23

echo "Trying to connect to $HOST on port $PORT..."

while true; do
    nc -z $HOST $PORT && break
    echo "Connection failed. Retrying in 2 seconds..."
    sleep 2
done

echo "Connection successful! Starting telnet session..."
telnet $HOST
