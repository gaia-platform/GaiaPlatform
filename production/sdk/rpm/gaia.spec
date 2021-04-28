# Note: this file is not being used by CPackRPM. CPackRPM uses its generated file.
Summary: Gaia
Name: Gaia
Version: @CPACK_PACKAGE_VERSION@
Release: 1
Group: Applications/System
License: Gaia License
Requires: libexplain51, libcap2, spdlog

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
