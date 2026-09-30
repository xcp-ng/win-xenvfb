XenVfb - The Xen Paravirtual Display Driver for Windows
=======================================================

The XenVfb package consists of a single device driver:

* xenvfb.sys is a kernel-mode display-only driver which attaches to a virtual
  device created by XenBus and exposes virtual monitors. It is also a protocol
  driver for the fbif wire protocol (see include\xen\io\fbif.h).

Quick Start Guide
=================

Building the driver
-------------------

See BUILD.md

Installing the driver
---------------------

See INSTALL.md

Driver Interfaces
=================

See INTERFACES.md

Miscellaneous
=============

For convenience the source repository includes some other scripts:

get_xen_headers.py
------------------

This will import any necessary headers from a given tag of that Xen
repository at git://xenbits.xen.org/xen.git.
