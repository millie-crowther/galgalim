FROM alpine:latest
RUN apk add --no-cache make cmake gcc musl-dev
COPY src /
COPY static /static
RUN cmake .
RUN make
CMD /galgalim
