FROM alpine:3.15.3

# install dependencies
RUN apk add --no-cache make cmake g++ musl-dev git libuv-dev libressl-dev zlib-dev libstdc++

WORKDIR "/"
RUN git clone https://github.com/datastax/cpp-driver.git
RUN cd cpp-driver && mkdir build && cd build && cmake .. && make && make install && cd / && rm -rf cpp-driver

# copy in source code and security key
COPY uriel/src /src
COPY common/src/* /src
COPY uriel/secure-connect-galgalim-db.zip /

# build app
WORKDIR "/src"
WORKDIR "/"
RUN cmake src
RUN make

# delete source code and uninstall packages to save on image size
RUN rm -rf /src
RUN apk del make cmake g++ git

# run app
CMD /uriel
