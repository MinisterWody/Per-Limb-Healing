Per-Limb Healing - Fallout 3/NV STATUS SWF
==================================================

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


Classic Geometry
----------------

The Classic paper doll is generated from the FO3/FNV SVG source assets in:

    Classic Source Assets/

The generator is repository-relative and updates only the generated vector
geometry in:

    Source/reconstructed-as/ClassicPaperDoll.as

SVG coordinates are preserved at a uniform 1/8 conversion. paperdoll.xml
supplies parent-relative placement and the complete doll is transformed into
the Fallout 4 STATUS layout without modifying Fallout 4 ConditionBoy assets.

The six interaction polygons are derived from the rendered limb silhouettes,
partitioned at shared joints, and use a non-zero invisible fill for robust
Scaleform hit testing. The same logical hovered-limb state is shared between
HP bars and paper-doll regions.

The Classic face is selected from the Pip-Boy CurrHP/MaxHP ratio, with critical
health taking priority, head condition able to force the injured expression,
and HeadFlags bit 0x02 providing the intoxication expression when applicable.


Calibrated Layout
-----------------

Paper doll root transform:

    x=-28.1, y=-139.6, scaleX=scaleY=1.100

Meter offsets from Bethesda timeline positions:

    Head       (0, 0)
    Torso      (0, -165)
    Left Arm   (0, -50)
    Right Arm  (+6, -50)
    Left Leg   (0, 0)
    Right Leg  (+6, 0)

Meter offsets are applied before StatusTab creates the interaction overlays, so
the hit regions inherit the calibrated bar positions.
