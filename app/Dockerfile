FROM alpine:latest
RUN apk add --no-cache make cmake gcc musl-dev
COPY src /
RUN cmake .
RUN make
CMD /galgalim
