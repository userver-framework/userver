ubuntu_image22.04:
	sudo docker build -t userver-builder:ubuntu-2204 -f DockerfileUbuntu.22.04 .

run_ubuntu22.04: ubuntu_image22.04
	sudo docker run -v $(shell pwd):/userver -it userver-builder:ubuntu-2204 bash

ubuntu_image21.10:
	sudo docker build -t userver-builder:ubuntu-2110 -f DockerfileUbuntu.21.10 .

run_ubuntu21.10: ubuntu_image21.10
	sudo docker run -v $(shell pwd):/userver -it userver-builder:ubuntu-2110 bash

ubuntu_image20.04:
	sudo docker build -t userver-builder:ubuntu-2004 -f DockerfileUbuntu.20.04 .

run_ubuntu20.04: ubuntu_image20.04
	sudo docker run -v $(shell pwd):/userver -it userver-builder:ubuntu-2004 bash

ubuntu_image18.04:
	sudo docker build -t userver-builder:ubuntu-1804 -f DockerfileUbuntu.18.04 .

run_ubuntu18.04: ubuntu_image18.04
	sudo docker run -v $(shell pwd):/userver -it userver-builder:ubuntu-1804 bash
