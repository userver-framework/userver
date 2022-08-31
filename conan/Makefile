ubuntu_image:
	sudo docker build -t userver-builder:ubuntu -f DockerfileUbuntu .

run_ubuntu:
	sudo docker run -v /home/popov/projects/github/userver/conan:/userver -it userver-builder:ubuntu bash
