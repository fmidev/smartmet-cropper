%define BINNAME cropper
Summary: cropper
Name: smartmet-%{BINNAME}
Version: 8.9.22
Release: 1.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 8.9.22-2, libsmartmet-imagine >= 8.9.22-1,  libjpeg, libjpeg-devel, libpng-devel,  zlib, zlib-devel, boost-devel
Requires: glibc, libgcc, libjpeg, libpng,  libstdc++, zlib
Provides: cropper cropper_auth

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
* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-1.el5.fmi
- Linked statically with boost 1.36
* Wed Jun 24 2008 oksman <pekka.keranen@fmi.fi> - 8.6.24-1.el5.fmi
- New compression quality and output format options
* Wed May 7 2008 oksman <santeri.oksman@fmi.fi> - 8.5.7-1.el5.fmi
- Fixed version naming convention
* Fri Apr 25 2008 oksman <santeri.oksman@fmi.fi> - 8.2.27-1.el5.fmi
- Fixed a bug that caused wrong content-length header
* Tue Feb 26 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.26-1.el5.fmi
- Fix output on authentication failures
* Mon Feb 25 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.2.25-2.el5.fmi
- Improvements in status code generation
* Thu Jan 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 8.1.24-1.el5.fmi
- Initial build

