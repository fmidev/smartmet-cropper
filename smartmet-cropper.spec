%define BINNAME cropper
%define RPMNAME smartmet-%{BINNAME}
Summary: CGI utility for cropping images
Name: %{RPMNAME}
Version: 24.2.29
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-cropper
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)

%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: smartmet-utils-devel >= 23.9.6
BuildRequires: smartmet-library-newbase-devel >= 24.2.23
BuildRequires: smartmet-library-imagine-devel >= 24.2.23
BuildRequires: %{smartmet_boost}-devel
Requires: smartmet-library-newbase >= 24.2.23
Requires: smartmet-library-macgyver >= 24.1.17
Requires: smartmet-library-imagine >= 24.2.23
Provides: cropper
Provides: cropper_auth
Obsoletes: libsmartmet-webauthenticator

%description
FMI cropper

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{RPMNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0775)
%{_bindir}/cropper
%{_bindir}/cropper_auth

%changelog
* Thu Feb 29 2024 Mika Heiskanen <mika.heiskanen@fmi.fi> - 24.2.29-1.fmi
- Improved safety

* Fri Jul 28 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.28-1.fmi
- Repackage due to bulk ABI changes in macgyver/newbase/spine

* Mon Jun 20 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.6.20-1.fmi
- Add support for RHEL9, upgrade libpqxx to 7.7.0 (rhel8+) and fmt to 8.1.1

* Tue May 24 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.24-1.fmi
- Repackaged due to NFmiArea ABI changes

* Fri May 20 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.20-1.fmi
- Repackaged due to ABI changes to newbase LatLon methods

* Mon Jan 24 2022 Andris Pavenis <andris.pavenis@fmi.fi> - 22.1.24-1.fmi
- Use makefile.inc from smartmet-utils-devel

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase API changes

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Fri Sep 27 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.27-1.fmi
- Repackaged due to ABI changes in SmartMet libraries

* Thu Jul 26 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.26-1.fmi
- Prefer nullptr over NULL

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Mon Feb 13 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.13-1.fmi
- Repackaged due to newbase API changes

* Thu Jan 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.12-1.fmi
- Switched to FMI open source naming conventions
- Merged webauthenticator library into this package

* Wed Jul  3 2013 mheiskan <mika.heiskanen@fmi.fi> - 13.7.3-1.fmi
- Update to latest versions of used libraries

* Mon Sep 29 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.29-1.fmi
- Newbase headers changed, everything is rebuilt

* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-1.fmi
- Linked statically with boost 1.36

* Tue Jun 24 2008 oksman <pekka.keranen@fmi.fi> - 8.6.24-1.fmi
- New compression quality and output format options

* Wed May 7 2008 oksman <santeri.oksman@fmi.fi> - 8.5.7-1.fmi
- Fixed version naming convention

* Fri Apr 25 2008 oksman <santeri.oksman@fmi.fi> - 8.2.27-1.fmi
- Fixed a bug that caused wrong content-length header

* Tue Feb 26 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.26-1.fmi
- Fix output on authentication failures

* Mon Feb 25 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.25-2.fmi
- Improvements in status code generation

* Wed Jan 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 8.1.24-1.fmi
- Initial build

