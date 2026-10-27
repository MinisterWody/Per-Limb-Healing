Per Limb Healing - Source
=========================

This repository contains the current Per Limb Healing source and production
artifacts for both supported Pip-Boy STATUS layouts.

Source layout
-------------
Plugin/
  Native F4SE plugin source, build script, and production DLL.

MCM/
  MCM configuration shared by both installer choices.

SWF - Fallout 4/
  Fallout 4 STATUS ActionScript source, vanilla SWF reference, build script,
  and production SWF.

SWF - Fallout 3-NV/
  Fallout 3/New Vegas-style STATUS ActionScript source, classic vector assets,
  vanilla SWF reference, build script, and production SWF.

Installer/
  Editable FOMOD XML and installer images.

SHA256SUMS.txt
  SHA-256 manifest for every file in this source tree except the manifest itself.

Build requirements
------------------
Native plugin:
- clang-cl and lld-link from an x64 development environment.
- F4SE headers expected by Plugin/build_plugin.cmd.

SWF:
- Apache Flex 4.16.1 / ASC 1.
- playerglobal32_0.swc.
- Place the required third-party tools under each SWF tree's tools/ directory.

The repository does not bundle third-party toolchains.
