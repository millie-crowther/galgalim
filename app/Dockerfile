FROM alpine:latest
RUN apk add --no-cache make gcc musl-dev
COPY src /
RUN make
CMD /main
