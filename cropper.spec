%define BINNAME cropper
Summary: cropper
Name: smartmet-%{BINNAME}
Version: 13.7.3
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 13.7.3-1
BuildRequires: libsmartmet-imagine >= 13.7.3-1
BuildRequires: libsmartmet-webauthenticator >= 13.7.3-1
BuildRequires: libjpeg
BuildRequires: libjpeg-devel
BuildRequires: libpng-devel
BuildRequires:  zlib
BuildRequires: zlib-devel
BuildRequires: boost-devel
Requires: glibc
Requires: libgcc
Requires: libjpeg
Requires: libpng
Requires: libstdc++
Requires: zlib
Provides: cropper
Provides: cropper_auth

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

