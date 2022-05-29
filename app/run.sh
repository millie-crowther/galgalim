if sh ./build.sh; then
    redis-server --daemonize yes
    docker run --env-file secrets/cassandra_env -p 8080:8080 galgalim 
    redis-cli shutdown
else
    echo "Build failure!"
fi