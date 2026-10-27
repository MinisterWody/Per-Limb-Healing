Per Limb Healing
================

Source repository for Per Limb Healing for Fallout 4.
Per Limb Healing separates player limb healing from normal Stimpak healing and provides direct limb treatment through the Pip-Boy STATUS page.

Two Pip-Boy STATUS layouts are included:
- Fallout 4
- Fallout 3/NV

Both layouts use the same native plugin and MCM configuration. Only the Pip-Boy STATUS SWF differs.

Repository Layout
-----------------

Plugin/
    Native F4SE plugin source and build script.
SWF - Fallout 4/
    ActionScript source and build resources for the standard Fallout 4
    Pip-Boy STATUS layout.
SWF - Fallout 3-NV/
    ActionScript source and build resources for the Fallout 3/New Vegas
    style STATUS layout.
MCM/
    Mod Configuration Menu files.
Installer/
    FOMOD installer configuration and preview images.
SHA256SUMS.txt
    SHA-256 checksums for the files in this source package.


Native Plugin
-------------
The native plugin is intentionally self-contained.

Build requirements:
- 64-bit Windows development environment
- clang-cl
- lld-link

The plugin does not require CommonLib, the F4SE SDK, F4SE headers, Address
Library files, or Windows SDK import libraries in order to compile and link.

The required Fallout 4 / F4SE ABI declarations are defined directly in the
plugin source.

Run:
    Plugin\build_plugin.cmd

The build script produces the native F4SE plugin DLL.

Address Library for F4SE Plugins is a runtime dependency of the installed mod.
It is not a compile-time dependency.


Pip-Boy SWF
-----------

The two STATUS layouts are maintained separately:
    SWF - Fallout 4
    SWF - Fallout 3-NV

Each directory contains the source and build resources for its respective
Pipboy_StatsPage.swf.

The SWF build process uses the preserved Fallout 4 Pip-Boy SWF as its base and
replaces the ActionScript bytecode while preserving the original non-ABC SWF
content.

See the README inside each SWF directory for its build instructions and
toolchain requirements.

Fallout 3/NV Layout Geometry
----------------------------

The Fallout 3/NV layout includes generated Classic STATUS paper-doll geometry.

Its geometry generator uses repository-relative paths and can regenerate the
geometry used by the current ClassicPaperDoll.as source.

Runtime Requirements
--------------------

The installed mod requires:

- Fallout 4
- F4SE
- Address Library for F4SE Plugins
- Mod Configuration Menu, for the MCM interface

Installer
---------

The FOMOD provides one STATUS-layout selection:
    Fallout 4
    Fallout 3/NV

Both options install the same native plugin and MCM files. The selected option
determines which Pipboy_StatsPage.swf is installed.

Checksums
---------

SHA256SUMS.txt contains SHA-256 hashes for the files distributed with this
source package.

On systems with sha256sum available, the package can be checked with:
    sha256sum -c SHA256SUMS.txt

License / Third-Party Material
------------------------------

Fallout 4 and related game assets are property of Bethesda Softworks.

This repository contains material intended for development of the Per Limb
Healing Fallout 4 mod. Any included third-party tools or resources remain
subject to their respective licenses.
