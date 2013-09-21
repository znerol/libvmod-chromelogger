=================
vmod_chromelogger
=================

--------------------------------
Varnish Module for Chrome Logger
--------------------------------

:Author: Lorenz Schori
:Date: 2013-09-21
:Version: 0.1
:Manual section: 3

SYNOPSIS
========

                import chromelogger;

                sub vcl_recv {
                    chromelogger.log("So long, and thanks for all the fish");
                }

                sub vcl_deliver {
                    ### Collect and encode all log entries
                    set resp.http.X-ChromeLogger-Data = chromelogger.collect();
                }


DESCRIPTION
===========

Varnish Module (vmod) for sending log entries to chrome logger


FUNCTIONS
=========

log
---

Prototype::

                log(STRING S)

Return value
	NONE
Description
    Record a log message and store it internally.

Example::

                chromelogger.log("So long, and thanks for all the fish");

collect
-------

Prototype::

                collect();

Return value
	REAL

Description
    Return all log entries and encode them for delivery to the chrome logger
    client.

Example::

                ### Collect and encode all log entries
                set resp.http.X-ChromeLogger-Data = chromelogger.collect();



INSTALLATION
============

If you received this packge without a pre-generated configure script, you must
have the GNU Autotools installed, and can then run the 'autogen.sh' script. If
you received this package with a configure script, skip to the second
command-line under Usage to configure.

Usage::

 # Generate configure script
 ./autogen.sh

 # Execute configure script
 ./configure VARNISHSRC=DIR [VMODDIR=DIR]

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``


SEE ALSO
========

* http://craig.is/writing/chrome-logger

COPYRIGHT
=========

This document is licensed under the same license as the
libvmod-chromelogger project. See LICENSE for details.

* Copyright (c) 2013 Lorenz Schori
