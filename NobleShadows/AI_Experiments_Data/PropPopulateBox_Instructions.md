# PropPopulateBox instructions (paste into the box's note)

Copy everything between the lines into the box's `Instructions` text (or one line per Actor Tag).
Claude finds every `BPA_PropPopulateBox` in the open level, reads this note on each one, and fills that box.

---
## GENERAL RULES (defaults — apply to every box unless the specific part overrides them)
- Stay inside the box bounds: x, y and height (stacked props included). The box is treated as axis-aligned.
- Floor = the surface straight below the box centre; props sit on it. If the floor is not flat, ask me.
- No overlap between props (stacking is the only exception, and only if STACK=yes).
- Random rotation (yaw) and slight random scale unless SCALE is given.
- Props go in their own outliner folder: `PropPopulate/<box label>`.
- Only touch the placed instances. Never edit the BPA_PropPopulateBox blueprint asset.
- Do not save, do not load levels, do not move the camera or change my selection.
- Leave the box as it is unless HIDE_BOX=yes.
- When done, report: what was placed, how many, anything that did not fit.

## SPECIFIC RULES (this box)
PROPS: <what goes in the box, e.g. barrels, pine trees, crates>
ASSETS: <optional: folder or exact mesh paths to use; otherwise Claude picks and tells me>
AMOUNT: <COUNT=40 | DENSITY=low/medium/high | FILL (as many as fit)>
STACK: <no | yes, max N high>
SCALE: <e.g. 0.8-1.2, or "natural">
ALIGN: <flat on floor | random tilt up to N degrees | leaning on walls>
KEEP_CLEAR: <optional: areas or paths to leave open>
SEED: <optional number, to get the same layout again>
HIDE_BOX: <no | yes>
NOTES: <anything else in plain words>
---

## How Claude reads it
1. Search the open level for actors of `BPA_PropPopulateBox_C`.
2. For each one, read in this order: an `Instructions` text/string variable if the blueprint has one, otherwise the actor's Tags (each tag one line like `PROPS=barrels`), otherwise the actor label.
3. Missing fields fall back to the general rules. Anything unclear gets one short question before placing.
4. Remove-and-redo: Claude only deletes actors in that box's own `PropPopulate/<label>` folder.
