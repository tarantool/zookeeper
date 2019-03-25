echo "Installing JRE"
wget --no-cookies --no-check-certificate --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" \
"https://download.oracle.com/otn-pub/java/jdk/8u202-b08/1961070e4c9b4e26a04e7f5a083f551e/jdk-8u202-linux-x64.rpm" -O /tmp/jre.rpm
sudo yum localinstall -y /tmp/jre.rpm

echo "Installing cloudera repo"
wget https://archive.cloudera.com/cdh5/one-click-install/redhat/${DIST}/x86_64/cloudera-cdh-5-0.x86_64.rpm -O /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo yum --nogpgcheck -y localinstall /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo yum install -y zookeeper zookeeper-server
sudo mkdir -p /var/lib/zookeeper
sudo chown -R zookeeper /var/lib/zookeeper/
sudo service zookeeper-server init
sudo service zookeeper-server start
