#!/bin/bash

find ./src -type f \( -name '*.o' -o -name '*.moc' \) -delete

docker run --volume="$PWD:$PWD" --user="$(id -u):$(id -g)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it ghcr.io/pgaskin/nickeltc:1.0 all koboroot
