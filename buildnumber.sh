#!/bin/sh

# Script to create the BUILDNUMBER used by compile-resource. This script
# needs the script createBuildNumber.pl to be in the same directory.

{ ./createBuildNumber.pl \
	tools/cdr2odt-build.stamp \
	tools/pub2odt-build.stamp \
	tools/vsd2odt-build.stamp \
	tools/wpd2odt-build.stamp \
	tools/wpg2odt-build.stamp \
	tools/wps2odt-build.stamp \
	libodfgen/libodfgen-build.stamp
#Success
exit 0
}
#unsucessful
exit 1
