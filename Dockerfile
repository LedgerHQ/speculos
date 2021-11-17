# This Dockerfile assembles an image with all the dependencies required to run
# speculos from the command-line (--display headless or --display console, no
# GUI).
#
# Prerequisite: a working speculos build.

FROM docker.io/library/python:3.9-slim

ADD . /speculos
WORKDIR /speculos

RUN pip install --upgrade pip pipenv
RUN pipenv install --deploy --system

RUN apt-get update && apt-get install -qy \
    qemu-user-static \
    libvncserver-dev \
    && apt-get clean

RUN apt-get clean && rm -rf /var/lib/apt/lists/

# default port for dev env
EXPOSE 1234
EXPOSE 1236
EXPOSE 9999
EXPOSE 40000
EXPOSE 41000
EXPOSE 42000

ENTRYPOINT [ "python", "./speculos.py" ]
