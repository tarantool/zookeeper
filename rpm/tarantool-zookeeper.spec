Name: tarantool-zookeeper
Version: 0.1
Release: 1%{?dist}
Summary: Tarantool Zookeeper module
Group: Applications/Databases
License: BSD
URL: https://github.com/tarantool/zookeeper
Source0: https://github.com/tarantool/zookeeper/archive/%{version}/zookeeper-%{version}.tar.gz
BuildRequires: cmake >= 2.8
BuildRequires: gcc >= 4.5
BuildRequires: tarantool >= 1.6.8.0
BuildRequires: tarantool-devel >= 1.6.8.0
BuildRequires: zookeeper-native >= 3.4.5
BuildRequires: openssl-devel
Requires: zookeeper-native >= 3.4.5
Requires: openssl

%description
Tarantool bindings to Zookeeper library

%prep
%setup -q -n zookeeper-%{version}

%build
%cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo
make %{?_smp_mflags}
#make -j2 test  # it fails for unknown reason

%install
%make_install

%files
%{_libdir}/tarantool/*/
%{_datarootdir}/tarantool/*/
%doc README.md
%{!?_licensedir:%global license %doc}
%license LICENSE

%changelog
* Wed Nov 22 2017 Igor Latkin <igorcoding@gmail.com> 0.1
- Initial version of the RPM spec
