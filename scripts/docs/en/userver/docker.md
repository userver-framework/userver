### Docker for userver

Docker is convenient to use as a ready-made container that contains the entire 
ecosystem for developing a server and services on it.

This page describes how to prepare and assemble a basic docker image

## Prepare data for image

The script @ref scripts/docker/prepare_image.sh "scripts/docker/prepare_image.sh" prepares 
the directory to build the docker image. It downloads some repositories from 
github and manages their dependencies.

To change the version of the library in docker provide that version from command line.: `AMQP_VERSION=v4.3.17 ./prepare_image.sh`
See full list of variables in @ref scripts/docker/prepare_image.sh "scripts/docker/prepare_image.sh"

## Build docker image

After successful execution of the script, you need to assemble the docker 
image with the command
```
docker build -t userver_docker_tag .
```
where `userver_docker_tag` should be replaced with the full path to the package,
if you want to upload the image to external storage, for example
`ghcr.io/userver-framework/ubuntu-userver-build-base:v1`.

## Set image in docker-compose.yml

In the file @ref docker-compose.yml "docker-compose.yml"
specify your image in the container you want to work with or create a new one

## Done

You can work with `userver` via `docker` using `docker-compose`, 
more_detailed @ref DOCKER_BUILD "here"

@example scripts/docker/prepare image.sh
@example docker-compose.yml
