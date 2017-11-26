echo "Installing Tarantool 1.7"

sudo yum clean all
sudo yum -y install http://dl.fedoraproject.org/pub/epel/epel-release-latest-${DIST}.noarch.rpm
sudo sed 's/enabled=.*/enabled=1/g' -i /etc/yum.repos.d/epel.repo
sudo rm -f /etc/yum.repos.d/*tarantool*.repo
sudo tee /etc/yum.repos.d/tarantool_1_7.repo <<- EOF
[tarantool_1_7]
name=EnterpriseLinux-${DIST} - Tarantool
baseurl=http://download.tarantool.org/tarantool/1.7/el/${DIST}/x86_64/
gpgkey=http://download.tarantool.org/tarantool/1.7/gpgkey
repo_gpgcheck=1
gpgcheck=0
enabled=1

[tarantool_1_7-source]
name=EnterpriseLinux-${DIST} - Tarantool Sources
baseurl=http://download.tarantool.org/tarantool/1.7/el/${DIST}/SRPMS
gpgkey=http://download.tarantool.org/tarantool/1.7/gpgkey
repo_gpgcheck=1
gpgcheck=0
EOF

sudo yum makecache -y --disablerepo='*' --enablerepo='tarantool_1_7' --enablerepo='epel'
sudo yum -y install tarantool

echo "Installing JRE"
wget --no-cookies --no-check-certificate --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" \
"http://download.oracle.com/otn-pub/java/jdk/8u151-b12/e758a0de34e24606bca991d704f6dcbf/jre-8u151-linux-x64.rpm" -O /tmp/jre.rpm
sudo yum localinstall -y /tmp/jre.rpm

echo "Installing cloudera repo"
wget https://archive.cloudera.com/cdh5/one-click-install/redhat/${DIST}/x86_64/cloudera-cdh-5-0.x86_64.rpm -O /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo yum --nogpgcheck -y localinstall /tmp/cloudera-cdh-5-0.x86_64.rpm
sudo yum install -y zookeeper zookeeper-server
sudo mkdir -p /var/lib/zookeeper
sudo chown -R zookeeper /var/lib/zookeeper/
sudo service zookeeper-server init
sudo service zookeeper-server start
