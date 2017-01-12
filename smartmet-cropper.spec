%define BINNAME cropper
%define RPMNAME smartmet-%{BINNAME}
Summary: CGI utility for cropping images
Name: %{RPMNAME}
Version: 17.1.12
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-cropper
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: smartmet-library-newbase-devel >= 17.1.10
BuildRequires: smartmet-library-imagine-devel >= 17.1.4
BuildRequires: boost-devel
Requires: smartmet-library-newbase >= 17.1.10
Requires: smartmet-library-imagine >= 17.1.4
Provides: cropper
Provides: cropper_auth
Obsoletes: libsmartmet-webauthenticator

%description
FMI cropper

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{BINNAME}
 
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
* Thu Jan 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.12-1.fmi
- Switched to FMI open source naming conventions
- Merged webauthenticator library into this package

* Wed Jul  3 2013 mheiskan <mika.heiskanen@fmi.fi> - 13.7.3-1.fmi
- Update to latest versions of used libraries

* Mon Sep 29 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.29-1.fmi
- Newbase headers changed, everything is rebuilt

* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-1.fmi
- Linked statically with boost 1.36

* Wed Jun 24 2008 oksman <pekka.keranen@fmi.fi> - 8.6.24-1.fmi
- New compression quality and output format options

* Wed May 7 2008 oksman <santeri.oksman@fmi.fi> - 8.5.7-1.fmi
- Fixed version naming convention

* Fri Apr 25 2008 oksman <santeri.oksman@fmi.fi> - 8.2.27-1.fmi
- Fixed a bug that caused wrong content-length header

* Tue Feb 26 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.26-1.fmi
- Fix output on authentication failures

* Mon Feb 25 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.25-2.fmi
- Improvements in status code generation

* Thu Jan 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 8.1.24-1.fmi
- Initial build

