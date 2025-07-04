SUMMARY = "A general-purpose utility library"
DESCRIPTION = "GLib is a general-purpose utility library, which provides many useful data types, macros, type conversions, string utilities, file utilities, a main loop abstraction, and so on."
HOMEPAGE = "https://developer.gnome.org/glib/"

# pcre is under BSD;
# docs/reference/COPYING is with a 'public domain'-like license!
LICENSE = "LGPL-2.1-or-later & BSD-3-Clause & PD"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c \
                    file://glib/glib.h;beginline=4;endline=17;md5=b88abb7f3ad09607e71cb9d530155906 \
                    file://gmodule/COPYING;md5=4fbd65380cdd255951079008b364516c \
                    file://gmodule/gmodule.h;beginline=4;endline=17;md5=b88abb7f3ad09607e71cb9d530155906 \
                    file://docs/reference/COPYING;md5=f51a5100c17af6bae00735cd791e1fcc"
BUGTRACKER = "http://bugzilla.gnome.org"
SECTION = "libs"

CVE_PRODUCT = "glib"

BBCLASSEXTEND = "native nativesdk"

DEPENDS = "glib-2.0-native \
           virtual/libintl \
           virtual/libiconv \
           libffi \
           libpcre \
           zlib"

PACKAGES += "${PN}-codegen ${PN}-utils"

LEAD_SONAME = "libglib-2.0.*"

inherit meson gettext gtk-doc pkgconfig ptest-gnome upstream-version-is-even bash-completion gio-module-cache manpages

DEPENDS:append:class-target = "${@' gtk-doc' if d.getVar('GTKDOC_ENABLED') == 'True' else ''}"

GTKDOC_MESON_OPTION = "gtk_doc"

S = "${WORKDIR}/glib-${PV}"

PACKAGECONFIG ??= "libmount \
                   ${@bb.utils.contains('PTEST_ENABLED', '1', 'tests', '', d)}"
PACKAGECONFIG[libmount] = "-Dlibmount=enabled,-Dlibmount=disabled,util-linux"
PACKAGECONFIG[manpages] = "-Dman=true, -Dman=false, libxslt-native xmlto-native"
# libelf is auto-detected without a configuration option
PACKAGECONFIG[libelf] = ",,elfutils"
PACKAGECONFIG[tests] = "-Dinstalled_tests=true,-Dinstalled_tests=false,"
PACKAGECONFIG[selinux] = "-Dselinux=enabled,-Dselinux=disabled,libselinux"

EXTRA_OEMESON = "-Ddtrace=false -Dfam=false -Dsystemtap=false"

do_configure:prepend() {
	sed -i -e '1s,#!.*,#!${USRBINPATH}/env python3,' ${S}/gio/gdbus-2.0/codegen/gdbus-codegen.in
}

FILES:${PN} = "${libdir}/lib*${SOLIBS} \
               ${libdir}/gio \
               ${libexecdir}/*gio-querymodules \
               ${datadir}/glib-2.0/schemas"

FILES:${PN}-utils += "${bindir}/glib-genmarshal \
                      ${bindir}/glib-gettextize \
                      ${bindir}/glib-mkenums \
                      ${bindir}/glib-compile-resources"

FILES:${PN}-dev += "${libdir}/glib-2.0/include \
                    ${libdir}/gio/modules/lib*${SOLIBSDEV} \
                    ${libdir}/gio/modules/*.la \
                    ${datadir}/glib-2.0/gettext/po/Makefile.in.in \
                    ${datadir}/glib-2.0/schemas/gschema.dtd \
                    ${datadir}/glib-2.0/valgrind/glib.supp \
                    ${datadir}/gettext/its"
FILES:${PN}-dbg += "${datadir}/glib-2.0/gdb ${datadir}/gdb"
FILES:${PN}-codegen = "${datadir}/glib-2.0/codegen/*.py \
                       ${bindir}/gdbus-codegen"
FILES:${PN}-utils = "${bindir}/*"

SHAREDMIMEDEP = "shared-mime-info"
SHAREDMIMEDEP:class-native = ""
# When cross compiling for Windows we don't want to include this
SHAREDMIMEDEP:mingw32 = ""

RRECOMMENDS:${PN} += "${SHAREDMIMEDEP}"

ARM_INSTRUCTION_SET:armv4 = "arm"
ARM_INSTRUCTION_SET:armv5 = "arm"
# Valgrind runtime detection works using hand-written assembly, which
# doesn't support mips16e
CPPFLAGS:append:class-target:mips16e = " -DNVALGRIND=1"

# GLib generally requires gettext to be present so for USE_NLS to yes.  For
# native builds as i18n is disabled globally we have to add a gettext-native dependency.
USE_NLS:class-target = "yes"
USE_NLS:class-nativesdk = "yes"
DEPENDS:append:class-native = " gettext-native"

EXEEXT = ""
EXEEXT:mingw32 = ".exe"

do_install:append () {
	if [ -f ${D}${bindir}/gtester-report ]; then
		sed ${D}${bindir}/gtester-report -i -e '1s|^#!.*|#!/usr/bin/env python3|'
	fi

	# Remove some unpackaged files
	rm -rf ${D}${datadir}/glib-2.0/codegen/__pycache__
	rm -f ${D}${datadir}/glib-2.0/codegen/*.pyc
	rm -f ${D}${datadir}/glib-2.0/codegen/*.pyo

	if [ -e ${D}${libdir}/charset.alias ]; then
		rm -f ${D}${libdir}/charset.alias
	fi

        # Make sure gio-querymodules is unique among multilibs
        if test "x${MLPREFIX}" != "x"; then
                mv ${D}${libexecdir}/gio-querymodules${EXEEXT} ${D}${libexecdir}/${MLPREFIX}gio-querymodules${EXEEXT}
        fi
        # Autotools does this, meson does not
        mkdir -p ${D}${libdir}/gio/modules
}

do_install:append:class-target () {
	# Tests are only installed on targets, not native builds.  Separating this out
	# keeps glib-2.0-native from depending on DISTRO_FEATURES
	if [ -f ${D}${datadir}/installed-tests/glib/gdbus-serialization.test ]; then
		if ${@bb.utils.contains("DISTRO_FEATURES", "x11", "false", "true", d)}; then
			rm ${D}${datadir}/installed-tests/glib/gdbus-serialization.test
		fi
	fi
	if [ -f ${D}${datadir}/installed-tests/glib/static-link.test ]; then
        if test "x${MLPREFIX}" != "x"; then
                mv ${D}${datadir}/installed-tests/glib/static-link.test ${D}${datadir}/installed-tests/glib/${MLPREFIX}static-link.test
        fi
	fi
}

# As we do not build python3 for windows, makes no sense to ship the script that's using it
do_install:append:mingw32() {
        rm -f ${D}${bindir}/gtester-report
}

CODEGEN_PYTHON_RDEPENDS = "python3 python3-distutils python3-xml"
CODEGEN_PYTHON_RDEPENDS:mingw32 = ""

RDEPENDS:${PN}-codegen += "${CODEGEN_PYTHON_RDEPENDS}"

#RDEPENDS:${PN}-ptest += "${PN}-utils"
#
#RDEPENDS:${PN}-ptest += "\
#            coreutils \
#            libgcc \
#            dbus \
#            gnome-desktop-testing \
#            tzdata \
#            tzdata-americas \
#            tzdata-asia \
#            tzdata-europe \
#            tzdata-posix \
#            shared-mime-info \
#            ${PN}-locale-ja \
#            ${PN}-locale-fr \
#            ${PN}-locale-el \
#            ${PN}-locale-hr \
#            ${PN}-locale-lt \
#            ${PN}-locale-pl \
#            ${PN}-locale-ru \
#            ${PN}-locale-th \
#            python3-core \
#            python3-modules \
#            ${@bb.utils.contains('GI_DATA_ENABLED', 'True', 'python3-dbusmock', '', d)} \
#            ${PN}-codegen \
#           "
#
#RDEPENDS:${PN}-ptest:append:libc-glibc = "\
#            glibc-gconv-utf-16 \
#            glibc-charmap-utf-8 \
#            glibc-gconv-cp1255 \
#            glibc-charmap-cp1255 \
#            glibc-gconv-utf-32 \
#            glibc-gconv-utf-7 \
#            glibc-gconv-euc-jp \
#            glibc-gconv-iso8859-1 \
#            glibc-gconv-iso8859-15 \
#            glibc-charmap-invariant \
#            glibc-localedata-translit-cjk-variants \
#            locale-base-tr-tr \
#            locale-base-lt-lt \
#            locale-base-ja-jp.euc-jp \
#            locale-base-fa-ir \
#            locale-base-ru-ru \
#            locale-base-de-de \
#            locale-base-hr-hr \
#            locale-base-el-gr \
#            locale-base-fr-fr \
#            locale-base-es-es \
#            locale-base-en-gb \
#            locale-base-en-us \
#            locale-base-pl-pl \
#            locale-base-pl-pl.iso-8859-2 \
#           "
