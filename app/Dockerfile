FROM alpine:3.15.3

# install dependencies
RUN apk add --no-cache make cmake g++ musl-dev git libuv-dev libressl-dev zlib-dev libstdc++

WORKDIR "/"
RUN git clone https://github.com/datastax/cpp-driver.git
RUN cd cpp-driver && mkdir build && cd build && cmake .. && make && make install && cd / && rm -rf cpp-driver

# copy in source code
COPY src /src

# build app
WORKDIR "/src"
RUN if [ -d hiredis ]; then echo "hiredis already present"; else git clone https://github.com/redis/hiredis.git; fi
WORKDIR "/"
RUN cmake src
RUN make

# delete source code and uninstall packages to save on image size
RUN rm -rf /src
RUN apk del make cmake g++ git

# copy in static files 
COPY static /static
COPY asset /asset

# run app
CMD /galgalim
