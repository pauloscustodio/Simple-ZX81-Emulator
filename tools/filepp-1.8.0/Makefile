########################################################################
#
# filepp is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
########################################################################
########################################################################
#
#  Project      :  File Pre Processor
#  Filename     :  $RCSfile: Makefile.in,v $
#  Originator   :  $Author: darren $
#  Maintainer   :  Darren Miller: darren@cabaret.demon.co.uk
#  File version :  $Revision: 1.34 $
#  Last changed :  $Date: 2007/02/13 00:06:54 $
#  Description  :  Main Makefile
#  Licence      :  GNU copyleft
#
########################################################################

##############################################################################
# Paths
##############################################################################
prefix = /mingw64
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
mandir = ${datarootdir}/man
srcdir = .
builddir = .
datarootdir = ${prefix}/share
moduledir = /mingw64/share/filepp/modules
# top level of test build directory
TESTBUILD = /home/paulo/git/Simple-ZX81-Emulator/tools/filepp-1.8.0/testbuild
module_subdirs = modules/bigdef modules/bigfunc modules/blc modules/case modules/cmacros modules/comment modules/cpp modules/defplus modules/for modules/foreach modules/format modules/function modules/grab modules/literal modules/maths modules/regexp
all_subdirs = $(module_subdirs) tests

##############################################################################
# Programs and files
##############################################################################
FILEPP     = $(builddir)/filepp
FILEPPMAN  = $(builddir)/filepp.1
FILEPPHTML = $(builddir)/filepp.html
FILEPPLSM  = $(builddir)/filepp.lsm
FILEPPSPEC = $(builddir)/filepp-1.8.0.spec
DISTRIBDIR = $(builddir)/filepp-1.8.0
DISTRIB    = $(DISTRIBDIR).tar.gz
CONFIGURE  = $(srcdir)/configure
ECHO       = echo
RM         = rm -fr
CP         = cp -pr
MKDIR      = $(srcdir)/mkinstalldirs
INSTALL    = /usr/bin/install -c
TAR        = tar
GZIP       = gzip
MAN2HTML   = man2html

##############################################################################
# Dependencies
##############################################################################

IFLAGS = -I$(srcdir)
DFLAGS = -DHIDDEN

default: $(FILEPP) $(FILEPPMAN)

$(FILEPP): $(FILEPPIN)
	$(CONFIGURE)

# man page
$(FILEPPMAN): $(srcdir)/filepp.1.in
	@$(ECHO) Processing $(srcdir)/filepp.1.in
	$(COMPILE.filepp) $(srcdir)/filepp.1.in -o $@

# lsm entry
$(FILEPPLSM): $(srcdir)/filepp.lsm.in

# spec entry
$(FILEPPSPEC): $(srcdir)/filepp.spec.in
	@$(ECHO) Processing $(srcdir)/filepp.spec.in
	$(COMPILE.filepp) -k $(srcdir)/filepp.spec.in -o $@

#########################################################################
# generic rule to make things in subdirectories
#########################################################################
dirmake:
	@for subdir in $(DIRECTORIES); do \
	  $(ECHO) ""; \
	  $(ECHO) "Entering directory $$subdir"; \
	  (cd $$subdir && $(MAKE) $(TARGET)) \
	   || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

# check, test, demo, whatever you wanna call it
test:	$(builddir)/test.html

check:	test

example:test

demo:	test

# full test of all modules and filepp
fulltest: testbuild test
	$(MAKE) dirmake DIRECTORIES="$(all_subdirs)" TARGET=test


$(builddir)/test.html: $(FILEPP) $(srcdir)/test.html.in \
	$(srcdir)/header.html.in $(srcdir)/footer.html.in $(srcdir)/testsuite

# makes a test version of filepp and installs it so it uses the correct
# versions of the modules (rather than any versions installed elsewhere)
.PHONY: testbuild
testbuild:
	$(MKDIR) $(TESTBUILD)
	(cd $(TESTBUILD) && /home/paulo/git/Simple-ZX81-Emulator/tools/filepp-1.8.0/configure --prefix=$(TESTBUILD) && $(MAKE) && $(MAKE) install)

# html version of man page
$(FILEPPHTML): $(FILEPPMAN)
	$(MAN2HTML) $(FILEPPMAN) > $(FILEPPHTML)

# installation
install: $(FILEPP) $(FILEPPMAN)
	@$(MKDIR) $(bindir)
	$(INSTALL) $(FILEPP) $(bindir)
	@$(MKDIR) $(mandir)/man1
	$(INSTALL) -m 644 $(FILEPPMAN) $(mandir)/man1
	$(MKDIR) $(moduledir)
	$(MAKE) dirmake DIRECTORIES="$(module_subdirs)" TARGET=install

# distribution
distrib: $(FILEPPMAN) $(FILEPPLSM) $(FILEPPHTML) $(FILEPPSPEC)
	-$(MKDIR) $(DISTRIBDIR)
	$(CP) $(FILEPPMAN)       $(FILEPPLSM)         $(FILEPPHTML) \
	$(FILEPPSPEC)            $(srcdir)/filepp.spec.in \
	$(srcdir)/filepp.lsm.in  $(srcdir)/filepp.1.in \
	$(srcdir)/Makefile.in    $(srcdir)/configure.in \
	$(srcdir)/COPYING        $(srcdir)/INSTALL    $(srcdir)/README \
	$(srcdir)/install-sh     $(srcdir)/configure  $(srcdir)/mkinstalldirs \
	$(srcdir)/filepp.in      $(srcdir)/ChangeLog \
	$(srcdir)/footer.html.in $(srcdir)/header.html.in \
	$(srcdir)/test.html.in   $(srcdir)/testsuite  $(srcdir)/testinc \
	$(srcdir)/testmod.pm \
	$(srcdir)/modules $(srcdir)/tests \
        $(DISTRIBDIR)
	$(TAR) czvf $(DISTRIB) $(DISTRIBDIR)
	$(RM) -r $(DISTRIBDIR)

# clean and distclean
clean:
	$(MAKE) dirmake DIRECTORIES="$(all_subdirs)" TARGET=clean
	$(RM) $(builddir)/test.html *~

distclean: clean
	$(MAKE) dirmake DIRECTORIES="$(all_subdirs)" TARGET=distclean
	$(RM) Makefile $(FILEPPLSM) $(FILEPP) $(FILEPPMAN) $(FILEPPHTML) \
	$(FILEPPSPEC) config.cache config.status config.log file $(DISTRIB) \
	$(TESTBUILD) autom4te.cache

##############################################################################
# Rules
##############################################################################

.SUFFIXES: .in .html.in .html .lsm.in .lsm

COMPILE.filepp  = $(FILEPP) $(DFLAGS) $(IFLAGS)

# rule to convert .html.in to .html
.html.in.html:
	@$(ECHO) Processing $<
	$(COMPILE.filepp) $< -o $@

# rule to convert lsm.in to .lsm
.lsm.in.lsm:
	@$(ECHO) Processing $<
	$(COMPILE.filepp) -k $< -o $@

##############################################################################
# End of file
##############################################################################
