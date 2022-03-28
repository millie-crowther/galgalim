FROM alpine:latest

# install dependencies
RUN apk add --no-cache make cmake gcc musl-dev

# copy in source code
COPY src /src

# build app
WORKDIR "/src"
RUN cmake .
RUN make

# delete source code to save on image size
RUN cp galgalim /galgalim
WORKDIR "/"
RUN rm -rf /src

# copy in static files 
COPY static /static

# run app
CMD /galgalim
