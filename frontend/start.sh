docker build --progress=plain --tag=galgalim_frontend .
docker run --name=galgalim_frontend_container galgalim_frontend
docker stop galgalim_frontend_container
docker rm galgalim_frontend_container
