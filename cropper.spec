Summary: cropper
Name: cropper
Version: 1.0.1
Release: 1.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}
Requires: smartmet-imagine => 1.0.1, smartmet-newbase >= 1.0.1-1, zlib >= 1.1.4, zlib-devel >= 1.1.4, libjpeg, libjpeg-devel, libpng-devel >= 1.2.2, libpng10 => 1.0, freetype >= 2.1.4, smartmet-webauthenticator => 1.0.1-1
Provides: cropper, cropper_auth

%description
FMI cropper

%prep
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT

%setup -q -n %{name}
 
%build
make clean
make depend
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
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 
- Initial build.

