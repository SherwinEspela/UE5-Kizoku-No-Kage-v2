# AnimStitch

A miniature video-editor-style timeline for animation clips. Drop animation sequences on a track, trim them, overlap them for crossfade blends, preview the result live on a character, and bake everything to a single animation sequence.

AnimStitch is editor-only, works with any skeleton, and never modifies your source animations.

**Engine:** Unreal Engine 5.8 (C++ plugin, `AnimStitch` runtime stub + `AnimStitchEditor` module).

## Install

1. Copy the `AnimStitch` folder into your project's `Plugins/` folder (or the engine's `Plugins/Marketplace/`).
2. Open the project. If the plugin is not enabled, enable **AnimStitch** in *Edit > Plugins > Animation* and restart.
3. Open the tool from **Window > Tools > AnimStitch**.

## 5-minute quickstart

1. **Open the tab.** Window > Tools > AnimStitch. A new, untitled timeline is ready.
2. **Pick a preview character.** Use the *Preview mesh* picker (only meshes that use the timeline's skeleton are offered). The right-hand **Content Browser** panel lists every animation sequence that uses that skeleton. Hover a sequence for a live preview and file info.
3. **Add clips.** Drag sequences from the right-hand panel (or the main Content Browser) onto the timeline, or right-click a sequence and choose *Add to Timeline*. Clips sit end to end. All clips must share one skeleton; anything else is rejected with a warning.
4. **Arrange and trim.**
   - Drag a clip body to move it or change its order.
   - Drag the clip edges to trim in/out (non-destructive).
   - Park the playhead and use **Split**, **Trim Start** or **Trim End** (buttons or right-click menu).
5. **Crossfade.** Drag a clip left over its neighbour: the overlap is a crossfade. Drag inside the overlap to resize it. Select the overlap and pick *Linear* or *Ease In/Out*. Right-click a clip > *Add Crossfade (default length)* uses the defaults from the plugin settings.
6. **Root motion.** Right-click the clip whose root motion should drive the result > *Root Motion Authority* (shown in purple). Later clips continue from where the authority left off instead of restarting at their own origin.
7. **Preview.** Play / Pause (Space), Stop, Loop, scrub by dragging the red playhead handle or the ruler. The green and red flags on the ruler set the playback range. Toggle **Ghost Joins** (pose at each join: orange = outgoing, blue = incoming), **Root Path** and **Grid**.
8. **Bake.** Press **Bake...**, choose the output folder and name, frame rate (0 = source rate) and root motion authority, then Bake. A progress dialog is shown and the result can be revealed in the Content Browser.

## Saving and reusing timelines

*Save* / *Save As* store the arrangement (clips, trims, crossfades, root authority, playback range) as an **AnimStitch Timeline** asset (Content Browser > Add > Animation). Double-click it, or use **Open...**, to continue editing. Ctrl+Z / Ctrl+Y work for all timeline edits.

## Batch Bake

Use a timeline as a template for many clips (for example the same *attack > idle* transition over many attacks):

1. Right-click the clip(s) to be replaced > *Batch Slot* (marked on the timeline).
2. Press **Batch Bake...**. For each batch slot, select animation sequences in the right-hand Content Browser panel and press *Add Selected*. Clip *N* of every slot is baked together into output *N*, so all slots need the same number of clips.
3. Choose the output folder, name prefix/suffix, frame rate and whether to overwrite, then Bake. The template's trims (clamped to each clip) or each clip's full length can be used.

## Foot lock (optional)

A post-blend correction that removes foot sliding on planted feet. It is **off by default**; turn it on with the **Foot Lock** switch in the tool's toolbar (next to Save / Bake). The preview and every bake then use it; the Bake window just shows whether it is on.

- **How it works:** after the timeline is blended, AnimStitch detects when each foot is planted (low enough and slow enough in world space, root motion included), pins the foot where it was planted, and re-solves the thigh/calf with two-bone IK. The foot keeps its world orientation, the knee keeps bending the same way, and the correction fades in and out at the ends of each plant. **Pelvis follows** (on by default, Settings dropdown): where a pinned foot would otherwise be beyond about 97% of the leg's reach, the pelvis (and spine/head with it) is moved the minimum amount that keeps the leg from being stretched straight, up to *Max offset* (20 cm by default), and eases back after an unlock. A leg that was already straight in the animation is never bent further. If the body moves farther away than the cap allows, the leg extends fully, so add an unlock. Flat ground is assumed.
- **Foot bones:** pick the left and right foot bones in the **Settings** dropdown beside the switch (they are guessed from bone names and remembered per skeleton in the plugin settings). The thigh and calf are the two parents above the foot bone.
- **Ground height** defaults to the height of the foot bones in the skeleton's reference pose. Thresholds (height, speed, minimum plant length, blend time) have sensible defaults and can be tuned in the same window.
- **Manual locks (persistent):** with foot lock on, two lanes (L / R) appear under the ruler. Automatic detection (amber) only *suggests* plants, and only until you touch a foot. *Click* the lane at a moment to lock the foot there (green tick): it stays glued to its world position at that moment until you click again to unlock it (red tick), there is no automatic or time-based release. *Drag* paints a region: a lock at the start and an explicit unlock at the end. From a foot's first manual mark on, only your marks decide for that foot; right-click the lane for *Lock Foot Here*, *Unlock Foot Here*, *Resume Auto-Detection Here* (hands the foot back to detection from that point) and *Clear Manual Locks*. A red line under the lane marks a stretch where a plant was detected but you released the foot. Marks are stored on the clip in source time, so they follow a clip when you move or trim it. If a locked foot is farther away than the leg can reach (the body moved on), the leg simply extends fully, so release the lock when the body leaves.- Works on the baked result only; your source animations are never touched. Animations authored in place (no root motion) have no world-space plants to lock.
## Predict Feet (experimental)

A second, separate switch next to **Foot Lock** in the toolbar, **off by default** and marked experimental. It makes *crossfades* plant-aware before Foot Lock looks at the result:

- Plants are detected on each source clip on its own (clean, unmixed motion), using the foot bones and thresholds from the Foot Lock settings.
- Inside every crossfade, a foot that is planted in the outgoing clip is held at that spot; where the incoming clip is planted, the foot settles onto the incoming clip's stance as the blend progresses; where both are planted the pin slides from the old stance to the new one.
- The pelvis is only read. When the pelvis moves farther than the leg can reach, a pin is released gracefully, and the pelvis is never moved by this pass.
- Only crossfades (overlaps) are affected; butt joins and everything outside the overlaps are left exactly as they were. It composes with Foot Lock: prediction runs first, Foot Lock cleans up what is left.
- The preview and the bake use the same code. Turn it off and the bake is identical to a plain blend.
## Bones, the Show menu and the Details panel

- **Show menu** (eye button, viewport top left), like the animation editor's: *Bones* submenu with **Sockets**, **Bone Names**, **Multi-Color Bones**, **Bone Draw Size** and the drawing modes **All Hierarchy / Selected Only / Selected and Parents / Selected and Children / Selected and Parents and Children / None**; and **Grid** (also Alt+Shift+G). The menu stays open while you toggle.
- Click a drawn joint to select that bone (clicking empty space clears the selection), or pick a bone from the dropdown at the viewport's top right.
- **Details** (a tab next to **Content Browser** in the right-hand panel): the selected bone's live transform at the current scrub time, in mesh space (includes root motion) and relative to its parent. Read-only. The values come from the pose the viewport shows, so foot lock, prediction and manual corrections are included.

## Manual bone corrections

Right-click **below the clip lanes** > *Add Bone Track* to add a Sequencer-style transform lane under the clips. Keyframes hold **additive** offsets, so they layer over the blended result instead of replacing it. Corrections are applied **after** foot prediction and foot lock (manual wins) and are baked into the output.

- **Core bone** (pelvis, spine, head, or *Other Bone...*): one control shape per bone. Move / rotate / scale it (buttons at the viewport's top left, or **W / E / R**) with the gizmo: each drag writes a key at the playhead. The first key of a track also adds a zero key at 0 s so the offset starts where you key it.
- **Limb FK**: three rotate-only tracks, a control on each joint.
- **Limb IK** (legs and arms): a control at the end effector plus a **pole vector** control for the knee/elbow direction; a two-bone IK solves the chain. The effector is a **free world-space target**, unparented from the root: a foot you key on the ground stays there while root motion plays, and the leg reaches until it is fully extended. Each key has an *IK weight*: 1 = on the target, 0 = animation. *Release IK Here* (lane menu) holds the plant, then eases back to the animation.
- **Foot-lock pins**: a manual lock in the foot lane shows its pin as a draggable control; dragging it moves the pinned spot (a free IK target shared with foot lock).
- Lane keys: click to select, drag to move, **Delete** to remove; right-click a lane for *Add Key at Playhead*, *Remove Track*, key weight and release options. Ctrl+Z / Ctrl+Y undo every edit.
## Scripting

`UAnimStitchBlueprintLibrary::BakeTimeline(Timeline, OutputAssetPath, FrameRate, bOverwriteExisting, OutError)` bakes a timeline asset from Blueprints, Editor Utility Widgets or Python.

## Settings

*Edit > Editor Preferences > Plugins > AnimStitch*: last preview mesh, default output folder, default bake frame rate, default crossfade length and curve.

## Keyboard shortcuts

| Key | Action |
| --- | --- |
| Space | Play / pause |
| Enter | Zoom timeline to fit |
| Delete | Remove selected clip |
| Ctrl+D | Duplicate selected clip |
| Ctrl+S | Save timeline |
| Ctrl+Z / Ctrl+Y | Undo / redo |
| Mouse wheel / Shift+wheel | Zoom / pan timeline |
| Middle mouse drag | Pan timeline |

## Limitations (v1)

- Only bone transforms are baked. Anim notifies and curves on the source clips are **not** carried over.
- One track; a clip can overlap only its immediate neighbours.
- Clips must all use the same skeleton.
- Montages and composites are not supported; use animation sequences.

## Notes for developers

- The same evaluator (`FAnimStitchEvaluator`) drives the live preview and the bake, so what you see is what is baked.
- Public API classes are exported with `ANIMSTITCHEDITOR_API`; all user-facing strings use `LOCTEXT`.
- `-run=AnimStitchSpike` is a headless development harness (bake/batch/asset round-trip checks) and is not needed by users.







## Viewport toolbar, controls and preview props
- The viewport has a toolbar: Move/Rotate/Scale icon buttons, a World/Local toggle, a Show menu (Grid, Bones, Controls with a Control Size slider, Preview Attachments) and a bone picker. Show-menu toggles repaint the viewport immediately.
- Adding a bone track adds keys at the timeline start and end (IK tracks start with weight 0).
- Preview Attachments (Show menu): attach static/skeletal meshes to a socket or any bone with an offset. Saved in the timeline asset, shown in the preview only, never baked.
- The transform manipulator is a modern-style thin, long gizmo drawn by the plugin (the engine gizmo cannot be hosted without an edit mode).

## Controls, gizmo and numeric key entry
- Control shapes are solid translucent cubes (the whole cube is clickable). Show > Controls has Control Size and Gizmo Size.
- Move / Rotate / Scale each remember their own World / Local choice.
- Dragging updates the pose live. A missed mouse release can no longer leave the gizmo stuck.
- Every bone-track lane has `<` / `>` buttons (previous / next key, moves the playhead) and a `v` button that opens a value panel under the lane: translate, rotate, scale (IK: target, rotate, pole, weight). With no key selected, edits key the playhead.
- Bone tracks have **Keep feet planted**: when the bone (pelvis...) moves the legs, both feet stay where the animation had them, solved with leg IK. A foot with its own active IK track is left to it. Out-of-reach feet lift (legs fully extended). FK limb tracks have it off.
- IK pole controls start at the knee/elbow pushed out along the bend direction; the fine 10 cm grid shows within 5 m of the origin.

## Timeline layout, transport and key clipboard
- The timeline has a Sequencer-style track column on the left: each bone track has an expand arrow (numeric translate / rotate / scale, or target / pole / weight for IK, plus Keep feet planted), previous key, add key and next key buttons. Keys are red circles, blue when selected.
- The bar under the timeline has go to start, previous key, play / pause, next key, go to end, stop, loop, a time or frame entry, a frames / seconds toggle and a pan / zoom range bar (drag the body to pan, the ends to zoom).
- Drag a rectangle over the correction lanes to select keys (Ctrl / Shift adds). Ctrl+C copies the selected keys, Ctrl+V pastes them onto the same bone tracks at the playhead keeping their relative timing, Delete removes them. Dragging moves one key at a time.
- Show > Controls: control cubes are translucent; the gizmo defaults to size 2 with thick lines.

## Phase 11 additions
- Undo / Redo buttons next to Duplicate / Remove (Ctrl+Z still works). Right-hand tabs: Content Browser, Details, **Baked Assets** (every sequence AnimStitch baked; double-click opens the Animation editor, right-click shows it in the Content Browser; stored in the AnimStitch editor settings).
- **Attach Props** button beside the preview mesh picker (same as Show > Preview Attachments).
- Control cubes are sized per bone (pelvis large, fingers small), coloured by side (left blue, right red, centre grey), rotate with the control and start aligned to the joint. Show > Controls: Control Size (global multiplier), Gizmo Size, Control Opacity. Their translucent fill is drawn on the viewport canvas so the mesh shows through.
- Rotate defaults to local. IK pole / effector controls stay on the animated limb while IK is off, so pelvis edits do not drag them.
- Timeline: wheel zooms (wheel over the track column scrolls the lanes), the lane column has a vertical scrollbar and a draggable width handle, Delete removes a selected track, adding a clip auto-keys every lane at the clip end, adding a duplicate bone shows a warning. Picking a bone in the viewport dropdown switches to the Details tab.
