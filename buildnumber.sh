#!/bin/sh

# Script to create the BUILDNUMBER used by compile-resource. This script
# needs the script createBuildNumber.pl to be in the same directory.

{ ./createBuildNumber.pl \
	cmdline/cdr2odt-build.stamp \
	cmdline/pub2odt-build.stamp \
	cmdline/vsd2odt-build.stamp \
	cmdline/wpd2odt-build.stamp \
	cmdline/wpg2odt-build.stamp \
	cmdline/wps2odt-build.stamp \
	filter/libodfgen-build.stamp
#Success
exit 0
}
#unsucessful
exit 1
