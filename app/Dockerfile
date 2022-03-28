FROM alpine:latest

# install dependencies
RUN apk add --no-cache make cmake gcc musl-dev

# copy in files
COPY src /src
COPY static /static

# build app
WORKDIR "/src"
RUN cmake .
RUN make

# delete source code to save on image size
RUN cp galgalim /galgalim
WORKDIR "/"
RUN rm -rf /src

# run app
CMD /galgalim
