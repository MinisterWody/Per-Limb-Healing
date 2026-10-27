Per-Limb Healing SWF source
===========================

Data/Interface/Pipboy_StatsPage.swf is compiled from:
  Source/SWF - Fallout 3-NV/Source/reconstructed-as/

The ActionScript is a real high-level source compile. No original method bodies
or P-code are grafted into the compiled ActionScript. The compiled ABC is grafted
onto the authoritative vanilla SWF timeline/asset baseline:
  Source/SWF - Fallout 3-NV/Reference/Pipboy_StatsPage-vanilla-reference.swf

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
- The Fallout 4 ConditionBoy assets are not modified or overridden; the Classic STATUS variant adds its own paper doll as a child of the existing STATUS host.
- The two targeted-Stimpak bridge operations share one F4SE/PerLimbHealing plugin-object lookup helper.
- The lookup and each native callback are capability-checked independently; a missing or partial native bridge makes the click a no-op instead of attempting an invalid AS3 call.

Classic geometry
- The alternate ClassicPaperDoll is generated from the supplied FO3/FNV SVG paths.
- SVG coordinates are preserved at a uniform 1/8 conversion; source canvas registration is not
  stretched to the paperdoll.xml width/height metrics.
- paperdoll.xml supplies parent-relative placement; a single root transform fits the complete
  classic doll to Fallout 4 STATUS.
- Shared Fallout 4 Interface/Components/ConditionClips assets remain untouched.

Final calibrated Classic STATUS layout
- Paper doll root transform: x=-28.1, y=-139.6, scaleX=scaleY=1.100.
- Meter offsets from Bethesda timeline positions: Head (0,0), Torso (0,-165),
  Left Arm (0,-50), Right Arm (+6,-50), Left Leg (0,0), Right Leg (+6,0).
- Meter offsets are applied before StatusTab creates PLH interaction overlays, so the
  clickable/hover regions inherit the calibrated bar positions.

Classic paper-doll limb interaction
- ClassicPaperDoll owns six invisible filled polygon hit regions rather than relying on thin vector-line artwork.
- Joint boundaries are shared exactly between neighbouring regions; the mesh has no positive-area overlaps.
- StatusTab registers these six regions with the same PLH hover/click handlers used by the HP bars.
- One logical hovered-limb state drives both surfaces: HP-bar hover pulses the matching classic limb, and classic-limb hover retains the normal bar feedback.
- The hovered classic limb pulses by smooth alpha/brightness modulation; Head includes the active face expression.
- Healthy limbs remain non-interactive. Clicking either the bar or the body region uses the existing targeted-Stimpak bridge.

Classic paper-doll hitbox coverage correction
- The six paper-doll interaction polygons are derived from the actual rendered SVG limb silhouettes, not hand-estimated anatomy.
- Each silhouette envelope receives a 1.25 local-unit edge allowance so anti-aliased visible edges remain interactive.
- Joint overlaps are partitioned before export; the six final event regions have zero positive-area overlap and shared boundaries.
- Paper-doll hit fills use alpha 0.001: visually imperceptible, but non-zero for robust Scaleform vector hit testing.

Classic health face mapping
- StatusTab passes Pip-Boy CurrHP and MaxHP to ClassicPaperDoll; no native/DLL data path was added.
- Health expression uses the clamped CurrHP/MaxHP ratio: >80% face_00, >60–80% face_01, >40–60% face_02, >20–40% face_03, <=20% face_04.
- Critical health (<=20%) is highest priority. Above critical health, headCondition < 1 forces face_03, so a crippled head can only show face_03 or face_04.
- On an uncrippled head above critical health, HeadFlags bit 0x02 selects face_10 as the classic intoxication override.
- Other HeadFlags bits do not directly choose the classic face.

Build
- build_source.py resolves all project paths relative to this SWF directory.
- Third-party Flex/playerglobal tools are not bundled.
