docker build --progress=plain --tag=galgalim_frontend .
docker run -p 80:80 --name=galgalim_frontend_container galgalim_frontend
docker stop galgalim_frontend_container
docker rm galgalim_frontend_container
