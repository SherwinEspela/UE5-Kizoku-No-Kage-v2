# ACBP_GrapplePointJumpSystem_v2 - Documentation

Last updated: 2026-10-15. Component path: `/Game/00_KNK_Assets/Blueprints/Components/ACBP_GrapplePointJumpSystem_v2`.
A snapshot copy of the component from 2026-10-15 is `ACBP_GrapplePointJumpSystem_v2_Backup_20261015` in the same folder (taken before the front-only search work; it is not updated afterwards).

---

## 1. What it is

An actor component for the player character that lets the player grapple-jump to a ledge or roof edge without any placed grapple points in the level.

It does four things:

1. **Finds a ledge.** A timer repeatedly scans in front of the camera for a roof or ledge edge the player can reach. The best one is the *selected point*.
2. **Shows an indicator.** A UI widget marks the selected point on screen.
3. **Runs the jump.** On the grapple input it launches the character, plays a *rise* montage, throws the kunai at the wall, then plays a *swing* montage and lands.
4. **Limits the camera.** While the jump is running, the camera yaw is limited to a cone around the target.

It is a duplicate of the older `ACBP_GrapplePointJumpSystem`, which is kept untouched as a backup. It works together with:

| Asset | Role |
|---|---|
| `ACBP_KunaiRopeGrappleSystem` (renamed from `ACBP_GrapplingHookSystem`) | Throws the kunai and rope, handles the wall hit effect. Has `ApplyJumpGrappleTarget` and `JumpWallHit` for this component. |
| `BPA_GrappleAnchor` | The kunai actor. Flies to the target, fires the wall-hit event on arrival. |
| `BPA_GrappleRope`, `BPA_GrappleImpactFx` | Rope and impact particle effect. |
| `WBP_GrappleTargetIndicator` | The on-screen indicator widget. |
| Animation notifies `ANB_GrappleThrow` / `ANB_GrappleSwingStart` | Empty, kept as assets. The user's two newer custom notifies (throw, retrieve) do the kunai timing. |
| `BPC_Player_v2`, `CBP_Kosuke_v2` | Player blueprints that own the component (see the naming note in section 6). |

Input: `IA_GrapplePointJump` in `PC_Gameplay` (Tab / L2) calls `EventITryGrapplePointJump`.

---

## 2. How a jump works (flow)

1. **Search loop** (`SearchLoop`, every `SearchInterval`): `RunLedgeSearchFront`, `SnapBestToGrid`, `DrawSearchDebug`, `UpdateIndicator`. Results land in the `Best*` variables.
2. **Input** -> `EventITryGrapplePointJump` (only if `BestFound` and not `InputLocked`):
   - traces into the wall just below the rim and stores `GrappleHit*`;
   - sets `InputLocked` true, `SearchEnabled` false, hides the indicator;
   - runs `SetGrappleWarpTargets`, `FaceGrappleTarget`, `PlayJumpMontage`.
3. **`PlayJumpMontage`:**
   - Calls `LaunchIfGrounded` (ground jumps) and starts the camera limit.
   - Computes the elevation angle from the player to `BestLandPoint`: `atan2(rise, horizontal distance)`.
   - Picks a band: **Steep** >= `SteepAngleMin`, **Shallow** < `ShallowAngleMax`, otherwise **Medium**. Ground jumps play that band's single rise montage (`RiseMontageSteep/Medium/Shallow`).
   - Air jumps use the `JumpMontagesAir*` arrays chosen by 3D distance (`ShortMaxDistance`, `MediumMaxDistance`).
4. **Rise** ends (timer = rise montage length) -> `BeginGrappleSwing`: sets Flying movement mode and plays a random montage from `SwingMontagesSteep/Medium/Shallow`. An empty swing array means the component goes straight to `RestoreWalking` at the end of the rise.
5. **Kunai:** the throw and retrieve notifies on the montages drive `ACBP_KunaiRopeGrappleSystem`. The anchor flies to the wall point, and on arrival fires the wall-hit event, which plays `JumpWallHit` (sound plus impact particle). No level collision or overlap setting is needed on the wall meshes.
6. **Landing:** `RestoreWalking` sets Walking mode, clears the debug timer, sets `InputLocked` false and `SearchEnabled` true.

### Launch rules (`LaunchIfGrounded`)
- Launch speed vertical = the character's `JumpZVelocity`. Horizontal = `GrappleJumpHorizontalSpeed`.
- Horizontal direction: movement input if there is any, else current velocity, else the direction to `BestLandPoint` (standing still).
- **Steep jumps** (angle >= `SteepAngleMin`): the part of the horizontal launch that points toward the target is removed. Sideways and backward movement is kept. A steep jump from standing still goes straight up.
- Why the swing montage must not have root motion while falling: a montage with a root-motion segment makes the character movement take its horizontal velocity from root motion, which is zero. That is why the jump is split into two montages (rise without root motion, swing with it).

---

## 3. Properties

All are on the component. **Important:** the component instances on the characters keep their own copy of every value. Changing a class default does not change them (see section 6).

### Search (`GrappleSearch|Search`)
| Property | Current value | Meaning |
|---|---|---|
| `SearchEnabled` | true | Master switch. Set false during a jump. |
| `SearchInterval` | 0.2 s | Time between searches. |
| `FrontOnlySearch` | true | true: `RunLedgeSearchFront` (camera cone only). false: old `RunLedgeSearch`. |
| `FrontRayCount` | 7 | Rays fanned across the cone. |
| `FrontScanHalfAngle` | 30 deg | Half width of the cone, around the camera yaw. |
| `FrontAnglePenalty` | 6 cm/deg | Score penalty per degree off center. Higher means the centered ledge wins more. |
| `RingCount` | 7 | Distance rings per ray. |
| `MinHorizontalDistance` | 150 cm | First ring distance. **A ledge edge closer than this to the player is rejected.** |
| `RingStep` | 250 cm | Distance between rings. Reach = `MinHorizontalDistance + (RingCount - 1) * RingStep` = 1650 cm. |
| `MinRise` / `MaxRise` | 300 / 1500 cm | Vertical band for a valid ledge, relative to the capsule bottom. |
| `StickyRadius` / `StickyBonus` | 300 / 200 cm | The previous target is kept unless a candidate is beyond the radius and closer by the bonus. |
| `SwitchCooldown` | 0 | Optional delay between target switches (off). |
| `PointSpacing` | 500 cm | `SnapBestToGrid` snaps points along the rim to this world grid (0 = off). |
| `AzimuthCount`, `FrontConeHalfAngle` | 16, 75 deg | **Old search only** (`RunLedgeSearch`). Not used when `FrontOnlySearch` is on. |

### Ledge and landing checks
`EdgeMarchStep` (60), `EdgeMarchSteps` (8), `MaxSurfaceStep` (40), `MinTopNormalZ`, `LandEdgeMargin` (100), `AnchorEdgeOffset`, `HeadroomPadding`: define how the rim is found and how much flat floor is needed behind it. Check the Details panel for their values; they were not changed during the 2026-10-15 work.

### Jump (`Default`)
| Property | Current value | Meaning |
|---|---|---|
| `SteepAngleMin` | 50 deg | Steep band start. |
| `ShallowAngleMax` | 20 deg | Shallow band end. Medium is in between. |
| `GrappleJumpHorizontalSpeed` | 500 | Horizontal launch speed. |
| `RiseMontageSteep/Medium/Shallow` | assign per character | Ground rise montages. |
| `SwingMontagesSteep/Medium/Shallow` | assign per character | Arrays; one is played at random. Empty = skip the swing. |
| `JumpMontagesAirShort/Medium/Long`, `ShortMaxDistance` (1000), `MediumMaxDistance` (1600) | assign | Air-jump montages, chosen by 3D distance to the land point. |
| `JumpMontagesShort/Medium/Long` | unused | Old ground arrays, kept for now. |
| `FaceInterpSpeed` | 4 | How fast the character turns toward the wall. |
| `AirFacingHalfAngle` | 60 deg | While airborne, only ledges within this angle of the character's facing are found. |
| `GrappleWallDrop` | 40 cm | How far under the rim the wall trace is made. |
| `GrappleTestDuration` | 5 s | Fallback unlock timer (`EndGrappleTest`). The normal unlock is `RestoreWalking`. |

### Camera (`GrappleJump|Camera`)
| Property | Current value | Meaning |
|---|---|---|
| `CameraYawLimit` | 45 deg | Max camera yaw away from the direction to the target during a jump. |
| `CameraYawSoftZone` | 15 deg | Last part of the range where the camera is gently pulled back. Beyond the limit the pull is stronger. |

### Indicator
`IndicatorEnabled`, `IndicatorHoldTime` (0.4 s), `IndicatorHeightOffset` (0), `IndicatorEdgeGap` (10 px), `IndicatorHalfSizeOverride`, `IndicatorHalfSize` (measured), `IndicatorWidget`, `IndicatorWorldPos`, `IndicatorShow`.

### Debug
`DebugMode`: 0 off, 1 draws search spheres (and the land sphere during a jump), 2 also shows rejection reasons. Keep 0 for normal play.

### Results and internal (do not edit)
- **Results** (`GrappleSearch|Result`): `BestFound`, `BestDistance`, `BestEdgePoint`, `BestAnchorPoint`, `BestAnchorNormal`, `BestLandPoint`, `GrappleHitPoint`, `GrappleHitNormal`, `GrappleHitValid`, `GrappleHitRotation`.
- **Internal:** `OwnerCharacter`, `RefsValid`, `InputLocked`, `PendingBand`, `Prev*`, `Scr*`, `LastFoundTime`, `LastSwitchTime`, `CapsuleHalfHeight`, `CapsuleRadius`.

---

## 4. Functions and events

| Name | What it does |
|---|---|
| `StartGrappleSearch` | Starts the `SearchLoop` timer (called from BeginPlay). |
| `ResolveSearchRefs` | Caches the owner character and capsule size. Disables the search if the owner is not a Character. |
| `SearchLoop` (event) | Runs the search each interval: calls `RunLedgeSearchFront` or `RunLedgeSearch`, then snap, debug draw, indicator. |
| `RunLedgeSearchFront` | **Current search.** Fans `FrontRayCount` rays across +-`FrontScanHalfAngle` of the camera yaw (from the player camera manager), checks each ring with `FindLedgeAtRay`, scores distance plus angle penalty, keeps the previous point while it is still valid and inside the cone. |
| `RunLedgeSearch` | **Old search.** 360 degree world-aligned azimuths, with an optional camera-cone penalty. Kept so `FrontOnlySearch` can switch back. |
| `FindLedgeAtRay(Dir, Dist)` | Core ledge check at one ray and distance: finds the top surface, marches back to the rim, checks flat floor, edge clearance, headroom and line of sight. Returns found, edge, anchor, anchor normal, land point, score. |
| `GatedDist`, `IsOutsideAirCone` | While airborne, rays outside `AirFacingHalfAngle` of the character's facing return nothing. |
| `SnapBestToGrid` | Snaps a newly chosen point along the rim to the `PointSpacing` grid. |
| `EnsureIndicator`, `UpdateIndicator`, `TickIndicator` | Create and move the indicator widget, clamped to the screen edge. |
| `DrawSearchDebug`, `DrawJumpDebug` | Debug drawing (`DebugMode`). |
| `EventITryGrapplePointJump` (event) | Grapple input handler (see section 2). |
| `SetGrappleWarpTargets` | Sets motion-warp targets `WarpGrappleSwingToLand` (`BestLandPoint`) and `WarpGrappleAnchorHandToWall` (`GrappleHitPoint` + yaw). Names must match the montage modifiers exactly. |
| `FaceGrappleTarget`, `TickFacing` | Slowly turns the character toward the wall; timer clears itself within 1 degree. |
| `PlayJumpMontage` | Picks and plays the montage (see section 2). |
| `LaunchIfGrounded` | Applies the launch (see "Launch rules"). Also starts the camera limit timer. |
| `BeginGrappleSwing` | Sets Flying and plays a random swing montage. |
| `RestoreWalking` | Landing/unlock: Walking mode, input unlocked, search enabled. |
| `TickCameraLimit` | Per-frame (0.016 s timer) camera yaw limit while `InputLocked`. Stops itself when input unlocks. |
| `EndGrappleTest` (event) | Fallback unlock after `GrappleTestDuration`. |
| `EventISetGrappleActorLocation`, `SetLandMotionWarpLocation` | Legacy from the old component, effectively unused. |

---

## 4b. References and interfaces

Interface assets live in `/Game/00_KNK_Assets/Blueprints/Interface/` (grapple ones in `Interface/Grapple/`). The lists below come from the interface assets themselves and from the component's graphs. The editor was offline when this section was written, so call sites outside this component (for example in the player controller) were not re-checked.

### Interface the component implements
**`BPI_GrapplePointJumpAbility`** (`Interface/Grapple/`). The player/controller reaches the component through this one.

| Interface function | In this component |
|---|---|
| `ITryGrapplePointJump` | Event `EventITryGrapplePointJump`. This is the grapple input entry point (see section 2). |
| `ISetGrappleActorLocation` | Event `EventISetGrappleActorLocation`. Legacy, empty. |
| `IAddGrappleJumpPoint`, `IRemoveGrappleJumpPoint`, `ISetGrappleJumpLocation`, `ISetGrappleAnchorTargetLocation`, `IGetGrappleJumpPointInterface` | Part of the old placed-point system. Not used by v2, which finds ledges by itself. |

`BPI_GrappleJumpPoint` (`IGetGrapplePointLocation`, `IGetGrappleLandPosition`, `IGetGrappleMidAirTransform`) belongs to the old placed jump-point actors. Not used by v2.

### Interface the player implements (the component calls it)
**`BPI_Player`** is held in the component variable `BPI_Player` (set in `ResolveSearchRefs`/BeginPlay).
- `IGetGrappleJumpAbility` returns this component, so other systems can find it without a cast.
- `ISetGPJ_LandMotionWarpLocation` is the legacy call behind `SetLandMotionWarpLocation`. This is the call that causes the existing compile error noted in section 6.
- `IGetMesh` and `GetPlayerLocation` are used by the kunai/rope (`ACBP_KunaiRopeGrappleSystem`, `BPA_GrappleAnchor`) for sockets and positions.
- `IBeginGrappleScan` / `IStopGrappleScan` and `IGrapple*` functions exist for the older grapple flow. v2 does not rely on them.

**`BPI_PCGameplay`** is held in the component variable `BPI_PCGameplay`. It is set on BeginPlay from the owning player controller. It drives the on-screen indicator: `IShouldDisplayGrappleTargetIndicator`, `ISetGrappleIndicatorOpaque`, `ISetGrappleIndicatorTransparent` (also `IGetTraceLocationsFromCamera`). If the controller does not implement this interface the indicator will not show.

**`BPI_PlayerAnimInstance`** is held in the component variable `BPI_PlayerAnimInstance`. It contains the older montage calls (`IPlayGrapplePointJump`, `IPlayGrappleThrow`, `IPlayGrappleSwing`, `IStopGrappleThrow`). v2 plays its montages directly on the mesh anim instance instead (`Montage Play`), so these are legacy.

### Interface messages sent to the owner character
**`BPI_GrappleAbility`** (`Interface/Grapple/`), sent to the character that owns the kunai component.
- `IThrowGrappleAnchor`: starts the kunai throw (spawns the anchor and rope). Triggered by the throw notify on the rise montage.
- `IHandleGrappleRetrieve`: pulls the kunai and rope back. Triggered by the retrieve notify, which is now in the swing montage.
- Other functions in the interface (`IHandleGrappleJustLanded`, `IHandlePointJumpFall`, `IGetGrapplingState`, `ISetGrapplingState`, `IGetHandRope`, climb/shimmy callbacks) belong to the older grapple/ledge system. The exact set used by the player's landing flow is handled by the user's own blueprints.

### Direct references
| Reference | Where it is used |
|---|---|
| `OwnerCharacter` (variable) | Cached character, set in `ResolveSearchRefs`. Everything in the search uses it. |
| `MotionWarpingComponent` | Found with `GetComponentByClass` in `SetGrappleWarpTargets`. |
| Mesh anim instance | Used by `PlayJumpMontage` / `BeginGrappleSwing` to play montages. |
| Player camera manager (index 0) | `RunLedgeSearchFront` reads the camera yaw from it. |
| Controller / control rotation | `TickCameraLimit` writes the yaw limit here. |
| `IndicatorWidget` (`WBP_GrappleTargetIndicator`) | Created by `EnsureIndicator` and added to the viewport. |
| `ACBP_KunaiRopeGrappleSystem` (kunai component) | Casts back to this component in `ApplyJumpGrappleTarget` to read `GrappleHitValid`, `GrappleHitPoint`, `GrappleHitNormal`, `InputLocked`. Sets its own `JumpGrappleActive`. |
| `BPA_GrappleAnchor` | Spawned by the kunai component. The anchor does not reference this component. |
| Legacy variables | `GrappleActorLocation`, `GrapplePoints`, `ClosestGrapplePoint` come from the old placed-point system and are unused. |

---

## 5. How to use

### Add it to a character
1. Add `ACBP_GrapplePointJumpSystem_v2` to the character blueprint.
2. The character needs a `MotionWarping` component, the mesh sockets `SocketRopeStart`, `SocketRopeBelt` and `SocketHandRope`, and the `ACBP_KunaiRopeGrappleSystem` component.
3. Assign the montages on the **component instance**: `RiseMontageSteep/Medium/Shallow` and `SwingMontagesSteep/Medium/Shallow` (plus the `JumpMontagesAir*` arrays if you want air jumps).
4. Make sure `IA_GrapplePointJump` is mapped in the player controller and calls the grapple interface.

### Set up the montages
- **Rise montages:** no root motion. Add your two kunai notifies (throw and retrieve) at the times you want. Retrieve must come after the kunai has had time to reach the wall.
- **Swing montages:** root motion allowed. Add the motion-warp windows using the two warp target names above.
- The swing begins when the rise montage ends, not from a notify.

### Tune it
- Change values on the **component instance** in the character blueprint (select the component, Details panel), and save the character blueprint.
- Reach too short/long: `RingCount`, `RingStep`, `MinHorizontalDistance`.
- Picks sideways points: lower `FrontScanHalfAngle` or raise `FrontAnglePenalty`.
- Montage band thresholds: `SteepAngleMin`, `ShallowAngleMax`.
- Camera too free/stiff: `CameraYawLimit`, `CameraYawSoftZone`.
- To see what the search sees: set `DebugMode` to 1 (2 for rejection reasons), then back to 0.

---

## 6. Notes, known issues and future considerations

### Gotchas
- **Instance values:** each character's component instance stores its own copy of the tunables. Changing class defaults does not update it. After adding a new variable the instance holds 0/false until set. Always set class and instances, then save the character blueprints.
- **Component name mix-up:** on `BPC_Player_v2` and `CBP_Kosuke_v2` this component is named `ACBP_KunaiRopeGrappleSystem` (its class is the jump system), and the kunai component is named `ACBP_GrapplingHookSystem`. This is left over from the rename. It works but is confusing.
- **Redirector:** the rename of `ACBP_GrapplingHookSystem` to `ACBP_KunaiRopeGrappleSystem` left a redirector. Use Fix Up Redirectors in the Content Browser and save.
- **Existing compile error** on `BPC_Player_v2` (old `SetLandMotionWarpLocation` call target, and an `IGetGrappleJumpAbility` cast with an empty object input) was left alone on request.
- The kunai hit is fired by the anchor when its flight finishes, only for jump grapples (the anchor's Owner is set by the kunai component in that case). The old camera-aim grapple is unchanged.

### Known issue
- **Camera glitch during the camera limit:** the yaw limit works, but the camera still glitches. Possible causes (not verified): the per-frame control rotation write fights the gameplay-camera rig or a control-rotation component, or the 0.016 s timer is not in step with the camera update. To investigate: how the camera rig reads yaw, and where the clamp should be applied (for example in the camera/controller update rather than a timer).

### Not done yet / ideas for a next version
1. **Interrupted jumps** (hit, early landing, cancel mid-swing): no handling. The unlock depends on timers (rise length, swing length, `RestoreWalking`). A blend-out delegate or an explicit cancel path is needed.
2. **A proper "landed" event** instead of timer-based unlock.
3. **Clean up unused assets:** old `RunLedgeSearch` and `AzimuthCount`/`FrontConeHalfAngle` once the front search is final; `JumpMontagesShort/Medium/Long`; legacy functions; the empty `ANB_GrappleThrow` and `ANB_GrappleSwingStart`.
4. **Move tuning into a data asset** so classes and character instances cannot drift apart.
5. **Search cost:** each search is `FrontRayCount x RingCount` ledge checks (49 now). Lower `SearchInterval` frequency or ring count if performance matters.
6. **Climbing/other ledge types** are not part of this component; it only handles flat rim-and-roof style ledges.
7. **Standing-still steep jumps** go straight up by design; a dedicated animation for "close to the wall" could look better than the normal steep rise.
