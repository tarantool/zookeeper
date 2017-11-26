echo "Installing Tarantool 1.7"

sudo rm -f /etc/yum.repos.d/*tarantool*.repo
sudo tee /etc/yum.repos.d/tarantool_1_7.repo <<- EOF
[tarantool_1_7]
name=Fedora-\$releasever - Tarantool
baseurl=http://download.tarantool.org/tarantool/1.7/fedora/\$releasever/x86_64/
gpgkey=http://download.tarantool.org/tarantool/1.7/gpgkey
repo_gpgcheck=1
gpgcheck=0
enabled=1

[tarantool_1_7-source]
name=Fedora-\$releasever - Tarantool Sources
baseurl=http://download.tarantool.org/tarantool/1.7/fedora/\$releasever/SRPMS
gpgkey=http://download.tarantool.org/tarantool/1.7/gpgkey
repo_gpgcheck=1
gpgcheck=0
EOF

sudo dnf -q makecache -y --disablerepo='*' --enablerepo='tarantool_1_7'
sudo dnf -y install tarantool

echo "Installing JRE"
wget --no-cookies --no-check-certificate --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" \
"http://download.oracle.com/otn-pub/java/jdk/8u151-b12/e758a0de34e24606bca991d704f6dcbf/jre-8u151-linux-x64.rpm" -O /tmp/jre.rpm
sudo dnf -y install /tmp/jre.rpm

echo "Installing cloudera repo"
wget https://archive.cloudera.com/cdh5/one-click-install/redhat/7/x86_64/cloudera-cdh-5-0.x86_64.rpm -O /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo dnf --nogpgcheck -y install /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo dnf install -y zookeeper zookeeper-server
sudo mkdir -p /var/lib/zookeeper
sudo chown -R zookeeper /var/lib/zookeeper/
sudo service zookeeper-server init
sudo service zookeeper-server start
