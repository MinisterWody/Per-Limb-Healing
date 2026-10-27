Per-Limb Healing - Fallout 4 STATUS SWF
===============================================

Source
------

The ActionScript source is located at:

    Source/reconstructed-as/

The authoritative timeline/asset baseline is:

    Reference/Pipboy_StatsPage-vanilla-reference.swf

The ActionScript is compiled from high-level source. No original method bodies
or P-code are grafted into the compiled ActionScript. The compiled ABC is then
grafted onto the vanilla SWF timeline/asset baseline so the original non-ABC
content is retained.


Toolchain
---------

Required:

- Java available on PATH
- Apache Flex 4.16.1 / ASC 1 (build 20171115)
- playerglobal32_0.swc

The repository does not bundle the Flex SDK or playerglobal SWC. Place them
under this SWF directory using this layout:

    tools/apache-flex/lib/mxmlc.jar
    tools/apache-flex/frameworks/
    tools/flashlib/playerglobal32_0.swc

Compiler target/settings:

- optimize=true
- strict=true
- omit-trace-statements=false
- Flash Player 11.2
- SWF version 15


Build
-----

From this directory run:

    python build_source.py

The script resolves all project paths relative to this directory, verifies the
vanilla SWF baseline hash, compiles the complete ActionScript source tree, and
writes the final file to:

    Built/Pipboy_StatsPage.swf

Compiler logs, the code-only intermediate SWF, and verification metadata are
also written under Built/ and are intended as local build outputs.


UI Architecture
---------------

- Pipboy_Meter contains no PLH interaction children.
- StatusTab creates six independent transparent interaction overlays after the
  vanilla timeline is constructed.
- Each overlay uses the meter's full original bounds plus 4 px padding, so the
  hitbox does not shrink with limb HP.
- A 2 px white source outline is shown only while hovering a damaged limb and
  inherits the normal Pip-Boy tint.
- Entering a damaged limb overlay dispatches BSScrollingList.PLAY_FOCUS_SOUND
  once per hover transition.
- Clicking a damaged limb adds no PLH pressed/selection visual or PLH click
  sound; Bethesda's native Stimpak feedback remains authoritative.
- Undamaged limbs are non-interactive and silent.
- ConditionBoy/Vault Boy is not modified.
- The two targeted-Stimpak bridge operations share one F4SE/PerLimbHealing
  plugin-object lookup helper.
- The lookup and each native callback are capability-checked independently; a
  missing or partial native bridge makes the click a no-op instead of attempting
  an invalid AS3 call.
