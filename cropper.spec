%define BINNAME cropper
Summary: cropper
Name: smartmet-%{BINNAME}
Version: 1.0.1
Release: 2.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 1.0.1-1, libsmartmet-imagine >= 1.0.1-1, libsmartmet-webauthenticator => 1.0.1-1, libjpeg, libjpeg-devel, libpng-devel,  zlib, zlib-devel
Requires: glibc, libgcc, libjpeg, libpng,  libstdc++, zlib
Provides: cropper, cropper_auth

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
* Fri Sep 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-2.el5.fmi
- Improved make system
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 1.0.1-1.el5.fmi
- Initial build.

