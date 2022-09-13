ubuntu_image:
	sudo docker build -t userver-builder:ubuntu -f DockerfileUbuntu .

run_ubuntu: ubuntu_image
	sudo docker run -v $(shell pwd):/userver -it userver-builder:ubuntu bash
