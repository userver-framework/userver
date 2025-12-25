#!/bin/sh

# convoluted setup of rabbitmq + erlang taken from https://www.rabbitmq.com/install-debian.html#apt-quick-start-packagecloud
## Team RabbitMQ's main signing key
curl -1sLf "https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E6026DFCA" | sudo gpg --dearmor | sudo tee /usr/share/keyrings/com.rabbitmq.team.gpg > /dev/null

## Add apt repositories maintained by Team RabbitMQ
sudo tee /etc/apt/sources.list.d/rabbitmq.list <<EOF
## Modern Erlang/OTP releases
##
deb [arch=amd64 signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://deb1.rabbitmq.com/rabbitmq-erlang/ubuntu/$(lsb_release -cs) $(lsb_release -cs) main
deb [arch=amd64 signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://deb2.rabbitmq.com/rabbitmq-erlang/ubuntu/$(lsb_release -cs) $(lsb_release -cs) main


## Provides modern RabbitMQ releases
##
deb [arch=amd64 signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://deb1.rabbitmq.com/rabbitmq-server/ubuntu/$(lsb_release -cs) $(lsb_release -cs) main
deb [arch=amd64 signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://deb2.rabbitmq.com/rabbitmq-server/ubuntu/$(lsb_release -cs) $(lsb_release -cs) main
EOF

## Update package indices
sudo apt update -y

# hackery to disable autostart at installation https://askubuntu.com/questions/74061/install-packages-without-starting-background-processes-and-services
mkdir /tmp/fake && ln -s /bin/true/ /tmp/fake/initctl && \
              ln -s /bin/true /tmp/fake/invoke-rc.d && \
              ln -s /bin/true /tmp/fake/restart && \
              ln -s /bin/true /tmp/fake/start && \
              ln -s /bin/true /tmp/fake/stop && \
              ln -s /bin/true /tmp/fake/start-stop-daemon && \
              ln -s /bin/true /tmp/fake/service && \
              ln -s /bin/true /tmp/fake/deb-systemd-helper
sudo PATH=/tmp/fake:$PATH apt install -y rabbitmq-server
