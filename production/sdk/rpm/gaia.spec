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
pkill -f -KILL gaia_db_server

%clean

%files
%defattr(-,root,root)
/opt/gaia/include/*
/opt/gaia/bin/*
/opt/gaia/lib/*
/opt/gaia/examples/*
/etc/opt/gaia/*

%changelog
* Mon Sep 14 2020  Gaia
- Initial version of the package
