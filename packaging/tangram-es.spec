Name:       tangram-es
Summary:    Tangram-ES Map Library
Version:    0.0.1
Release:    1
Group:      Framework/maps
License:    MIT
Source0:    %{name}-%{version}.tar.gz
Source1:    deps.tar.gz

BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires: 	pkgconfig(libcurl)
BuildRequires: 	pkgconfig(icu-uc)
BuildRequires: 	pkgconfig(freetype2)
BuildRequires: 	pkgconfig(harfbuzz)
BuildRequires: 	pkgconfig(evas)
BuildRequires: 	pkgconfig(fontconfig)

Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig

%ifarch %arm
%define ARCH arm
%endif

%ifarch aarch64
%define ARCH aarch64
%endif

%ifarch %ix86
%define ARCH i586
%endif

%ifarch x86_64
%define ARCH x86_64
%endif

%description
Tangram-ES Map Library.

%prep
%setup -q

rmdir external/alfons
rmdir external/geojson-vt-cpp
rmdir external/yaml-cpp
rmdir core/include/glm
rmdir core/include/variant
rmdir core/include/isect2d
rmdir core/include/earcut.hpp
rmdir external/duktape

%setup -q -T -D -a 1


%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE -g"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE -g"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
#echo "-------------------------------->>"
#find .
#echo "--------------------------------<<"
cmake .  -DCMAKE_BUILD_TYPE=Debug -DARCH=%{ARCH} -DPLATFORM_TARGET=tizen-lib -DCMAKE_INSTALL_PREFIX=%{_prefix} -DMAJORVER=${MAJORVER} -DFULLVER=%{version}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
# cp external/yaml-cpp/LICENSE %{buildroot}/usr/share/license/%{name}-yaml-cpp
# ... or embed the other licenses in LICENSE

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%manifest tangram-es.manifest
%defattr(-,root,root,-)
%{_libdir}/libtangram.so
/usr/share/license/tangram-es
