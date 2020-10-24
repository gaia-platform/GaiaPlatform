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
/usr/local/lib/librocksdb.so
/usr/local/lib/libgaia_direct.a
/usr/local/lib/libgaia_rules.a
/usr/local/lib/libgaia.a
/usr/local/lib/libgaia_parser.a
/usr/local/lib/libgaia_catalog.a
/usr/local/lib/libgaia_common.a
/usr/local/lib/libgaia_payload_types.a
/usr/local/lib/libgaia_se_client.a
/usr/local/lib/librocks_wrapper.a
/usr/local/lib/libflatbuffers.a

%changelog
* Mon Sep 14 2020  Gaia
- Initial version of the package
