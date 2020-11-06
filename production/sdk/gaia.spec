Summary: Gaia
Name: Gaia
Version: 1.0.0
Release: 1
Group: Applications/System
License: Gaia License

%description
Gaia System

%prep

%build

%install

%pre
pkill -f -KILL gaia_se_server

%clean

%files
%defattr(-,root,root)
/usr/local/include/gaia/*
/usr/local/bin/gaia_se_server
/usr/local/bin/gaiac
/usr/local/bin/gaiat
/usr/local/lib/libclang.so
/usr/local/lib/libgaia_storage.so
/usr/local/lib/se_client.cpython-38-x86_64-linux-gnu.so
/usr/local/lib/libgaia_index.so
/usr/local/lib/libgaia_memorymanager.so
/usr/local/lib/libgaia_triggers.so
/usr/local/lib/gaia_fdw-0.0.so
/usr/local/lib/librocksdb.so.6
/usr/local/lib/libgaia.so
/usr/lib/x86_64-linux-gnu/libexplain.so.51
/usr/local/lib/libfmt.so.7
/usr/local/lib/libspdlog.so.1
/usr/lib/x86_64-linux-gnu/libcap.so.2
/usr/lib/x86_64-linux-gnu/libcap-ng.so.0

%changelog
* Mon Sep 14 2020  Gaia
- Initial version of the package
