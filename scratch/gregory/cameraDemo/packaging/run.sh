#!/usr/bin/env bash

set -e

ORIGINAL_CWD=$CWD
trap 'dirs -c && cd $ORIGINAL_CWD' exit

pushd "$(dirname "$0")"
PACKAGING_DIR=$(pwd)
REPO_ROOT_DIR=$(git rev-parse --show-toplevel)
popd
echo "packaging dir: $PACKAGING_DIR" 1>&2
echo "repo root dir: $REPO_ROOT_DIR" 1>&2

pushd "$REPO_ROOT_DIR"
docker build --shm-size=1gb -f "$PACKAGING_DIR/Dockerfile" . --tag camera_demo
popd

pushd "$ORIGINAL_CWD"
DOCKER_CONTAINER_ID=$(docker create camera_demo)
docker cp "$DOCKER_CONTAINER_ID":"/CameraDemo-$(arch).AppImage" .
docker container rm -f "$DOCKER_CONTAINER_ID" || true
popd

echo "Created ./CameraDemo-$(arch).AppImage" 1>&2
