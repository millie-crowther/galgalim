if sh ./build.sh; then
    docker run --env-file ../secrets/cassandra_env -p 80:80 uriel 
else
    echo "Build failure!"
fi