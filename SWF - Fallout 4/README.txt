Per-Limb Healing SWF source
===========================

Data/Interface/Pipboy_StatsPage.swf is compiled from:
  Source/SWF - Fallout 4/Source/reconstructed-as/

The ActionScript is a real high-level source compile. No original method bodies
or P-code are grafted into the compiled ActionScript. The compiled ABC is grafted
onto the authoritative vanilla SWF timeline/asset baseline:
  Source/SWF - Fallout 4/Reference/Pipboy_StatsPage-vanilla-reference.swf

Toolchain
- Apache Flex 4.16.1 / ASC 1 (build 20171115)
- playerglobal32_0.swc
- optimize=true, strict=true, omit-trace-statements=false
- target Flash Player 11.2 / SWF 15

Current UI architecture
- Pipboy_Meter contains no PLH interaction children.
- StatusTab creates six independent transparent interaction overlays after the vanilla timeline is constructed.
- Each overlay uses the meter's full original bounds plus 4 px padding, so the hitbox does not shrink with limb HP.
- A 2 px white source outline is shown only while hovering a damaged limb and inherits the normal Pip-Boy tint.
- Entering a damaged limb overlay dispatches BSScrollingList.PLAY_FOCUS_SOUND once per hover transition.
- Clicking a damaged limb adds no PLH pressed/selection visual or PLH click sound; Bethesda's native Stimpak feedback remains authoritative.
- Undamaged limbs are non-interactive and silent.
- ConditionBoy/Vault Boy is not modified.
- The two targeted-Stimpak bridge operations share one F4SE/PerLimbHealing plugin-object lookup helper.
- The lookup and each native callback are capability-checked independently; a missing or partial native bridge makes the click a no-op instead of attempting an invalid AS3 call.

Build
- build_source.py resolves all project paths relative to this SWF directory.
- Third-party Flex/playerglobal tools are not bundled.
