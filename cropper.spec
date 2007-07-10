Summary: cropper
Name: cropper
Version: 1.0
Release: 1
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}
Requires: imagine => 1.0-1, newbase >= 1.0-1, zlib >= 1.1.4, zlib-devel >= 1.1.4, libjpeg, libjpeg-devel, libpng-devel >= 1.2.2, libpng10 => 1.0, freetype >= 2.1.4, webauthenticator => 1.0-1

%description
FMI cropper

%prep
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT

%setup -q -n %{name}
 
%build
make %{_smp_mflags} 

%install
make install prefix="${RPM_BUILD_ROOT}"
mkdir -p ${RPM_BUILD_ROOT}/smartmet/src/c++/bin/cropper
cp -r . ${RPM_BUILD_ROOT}/smartmet/src/c++/bin/cropper

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,www,www,0775)
/smartmet/src/c++/bin/cropper
/usr/bin/cropper
/usr/bin/cropper_auth

%changelog
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 
- Initial build.

