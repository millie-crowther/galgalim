./build.sh
redis-server --daemonize yes
docker run -p 8080:8080 galgalim
redis-cli shutdown