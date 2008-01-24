%define BINNAME cropper
Summary: cropper
Name: smartmet-%{BINNAME}
Version: 8.1.24
Release: 1.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 1.0.5-1, libsmartmet-imagine >= 1.0.4-1,  libjpeg, libjpeg-devel, libpng-devel,  zlib, zlib-devel
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
* Thu Jan 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 8.1.24-1.el5.fmi
- Initial build

