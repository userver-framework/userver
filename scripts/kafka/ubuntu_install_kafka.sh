sudo apt install -y default-jre

curl https://dlcdn.apache.org/kafka/3.8.0/kafka_2.13-3.8.0.tgz -o kafka.tgz
sudo mkdir -p /etc/kafka
sudo tar xf kafka.tgz --directory=/etc/kafka
sudo cp -r /etc/kafka/kafka_2.13-3.8.0/* /etc/kafka/
sudo rm -rf /etc/kafka/kafka_2.13-3.8.0
