Per Limb Healing
================

Source repository for Per Limb Healing for Fallout 4.

Per Limb Healing separates player limb healing from normal Stimpak healing and
provides direct limb treatment through the Pip-Boy STATUS page.

Two Pip-Boy STATUS layouts are included:

- Fallout 4
- Fallout 3/NV

Both layouts use the same native plugin and MCM configuration. Only the
Pip-Boy STATUS SWF differs.


Repository Layout
-----------------

Plugin/
    Native F4SE plugin source, build script, and current DLL.

SWF - Fallout 4/
    ActionScript source, build resources, and current SWF for the standard
    Fallout 4 Pip-Boy STATUS layout.

SWF - Fallout 3-NV/
    ActionScript source, Classic STATUS geometry assets, build resources, and
    current SWF for the Fallout 3/New Vegas-style STATUS layout.

MCM/
    Mod Configuration Menu files shared by both layouts.

Installer/
    FOMOD installer configuration, preview images, and packaging script.

SHA256SUMS.txt
    SHA-256 checksums for the files in this repository, excluding the manifest
    itself.


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

Run from the repository root or from Plugin/:

    Plugin\build_plugin.cmd

The build script writes:

    Plugin\build\PerLimbHealing.dll

Address Library for F4SE Plugins is a runtime dependency of the installed mod.
It is not a compile-time dependency.


Pip-Boy SWF
-----------

The two STATUS layouts are maintained separately:

    SWF - Fallout 4
    SWF - Fallout 3-NV

Each directory contains the source and build resources for its respective
Pipboy_StatsPage.swf.

The SWF build process compiles the reconstructed ActionScript and replaces the
ABC bytecode in the preserved Fallout 4 Pip-Boy SWF baseline while retaining
its non-ABC timeline and asset tags.

SWF build requirements:

- Java available on PATH
- Apache Flex 4.16.1 / ASC 1
- playerglobal32_0.swc

For either SWF directory, place the external tools at:

    tools/apache-flex/lib/mxmlc.jar
    tools/apache-flex/frameworks/
    tools/flashlib/playerglobal32_0.swc

Then run:

    python build_source.py

The final file is written as:

    Built/Pipboy_StatsPage.swf

See the README inside each SWF directory for layout-specific details.


Fallout 3/NV Layout Geometry
----------------------------

The Fallout 3/NV layout includes generated Classic STATUS paper-doll geometry.
The geometry generator uses repository-relative paths and regenerates the
geometry used by the current ClassicPaperDoll.as source.


FOMOD Installer
---------------

The FOMOD provides one STATUS-layout selection:

    Fallout 4
    Fallout 3/NV

Both choices install the same native plugin and MCM files. The selected option
determines which Pipboy_StatsPage.swf is installed.

To create the release installer from the repository contents, run:

    python Installer/package_fomod.py

The packaging script reads the version from Installer/fomod/info.xml and writes
a deterministic FOMOD ZIP at the repository root.


Runtime Requirements
--------------------

The installed mod requires:

- Fallout 4
- F4SE
- Address Library for F4SE Plugins
- Mod Configuration Menu, for the MCM interface


Checksums
---------

SHA256SUMS.txt contains SHA-256 hashes for the files distributed in this
repository. On systems with sha256sum available:

    sha256sum -c SHA256SUMS.txt


Third-Party Material
--------------------

Fallout 4 and related game assets are property of Bethesda Softworks.
Third-party tools and resources remain subject to their respective licenses.
