Summary: Gaia
Name: Gaia
Version: 0.1.0
Release: 1
Group: Applications/System
License: Gaia License
Requires: libexplain51, libcap2

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
/usr/local/bin/*
/usr/local/lib/*

%changelog
* Mon Sep 14 2020  Gaia
- Initial version of the package
