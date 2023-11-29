#!/bin/bash
n=$(cat StatusQueue.txt | grep $1 | awk '{print $3}')

