FROM alpine:3.15.3

# install dependencies
RUN apk add --no-cache make cmake gcc musl-dev git

# copy in source code
COPY src /src

# build app
WORKDIR "/src"
RUN if [ -d hiredis ]; then rm -rf hiredis; fi && git clone https://github.com/redis/hiredis.git
WORKDIR "/"
RUN cmake src
RUN make

# delete source code and uninstall packages to save on image size
RUN rm -rf /src
RUN apk del make cmake gcc git

# copy in static files 
COPY static /static
COPY asset /asset

# run app
CMD /galgalim
