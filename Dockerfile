# vault org api docker file
FROM python:3.8-slim

ADD . /app
WORKDIR /app
# Single RUN for Single stage without multistage without squash
RUN /app/scripts/docker_install.sh
# default port for dev env
EXPOSE 1234
EXPOSE 50000
EXPOSE 60000
CMD [ "/app/run.sh" ]
