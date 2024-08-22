#!/bin/bash

# Запуск dev контейнера. 
# ./dev.sh – билдит и запускает контейнер. 
#   Если запущенный контейнер с таким именем уже есть, то использует его (без ребилда).
# ./dev.sh 1 – билдит и запускает конейнер всегда с ребилдом.

RUNNING_CONTAINER_ID=$(docker ps -q --filter ancestor=umfs-workspace --filter status=running)

if [ "$RUNNING_CONTAINER_ID" != "" ] && [ "$1" == "" ]; then
    docker exec -it $RUNNING_CONTAINER_ID bash
    exit 0
fi

docker rm -f -v $(docker ps --filter ancestor=umfs-workspace -q)

docker build -t umfs-workspace . 
 
DEV_CONTAINER_ID=$(docker run -it -d -v .:/root/umfs umfs-workspace)

echo $DEV_CONTAINER_ID

docker exec -it $DEV_CONTAINER_ID bash
