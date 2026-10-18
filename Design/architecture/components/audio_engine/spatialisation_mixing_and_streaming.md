# Spatialisation, mixing and streaming

> Status: Engine baseline · Prefix: AEN · ADRs: [ADR-024](../../../ADR/ADR-024-audio-buses-and-per-bus-effects.md) · [ADR-025](../../../ADR/ADR-025-audio-formats-codecs-and-residency.md) · [ADR-027](../../../ADR/ADR-027-spatialisation-and-distance-attenuation.md) · [ADR-058](../../../ADR/ADR-058-pack-files-and-mount-policy.md) · Related: [Audio engine](README.md) · [Instances, voices and envelopes](instances_voices_and_envelopes.md) · [positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md) · [asset pipeline and resources](../asset_pipeline_and_resources.md)

This document specifies how a real voice is placed and mixed and where its samples come from: the distance gain and panning, the volume law and gain stages, the bus graph with its routing and effects, the residency of resident banks, streaming, and device loss and restart. The notation and the units are those of the [folder README](README.md), and section numbers are those of the whole component.

## 1 Responsibilities and boundaries

The responsibilities and boundaries of the audio engine are [README](README.md) §1.

## 2 Interfaces

The interface sketch, the commands, the inputs and the voice back-end contract are [README](README.md) §2.

## 3 Data model

The types, the layer data, the instance state, the effect kinds and the stream format are [README](README.md) §3.

## 4 Behaviour and algorithms

### 4.14 Spatialisation

**AEN-201** (MUST) **Distance gain** (ADR-027, R-AUD-202). For a positioned real or virtual instance, d = \|emitter position − listener position\| in wu and md is its layer's minimum distance: g(d) = 1 for d ≤ md and md ÷ d otherwise.

**AEN-202** (MUST) **Maximum distance.** G(d) = g(d) × h(d), with dmax the event's maximum distance (AEN-062) and f = `aen.spatial.max_distance_fade_fraction` × dmax: h = 1 for d ≤ dmax − f, (dmax − d) ÷ f for dmax − f < d < dmax, and 0 for d ≥ dmax. The global default `aen.spatial.max_distance_default` lies beyond every distance in a map, so with the defaults G = g and the default mix keeps DEC-566's default of no maximum distance; a per-event maximum is the mechanism of DEC-566's option B.

**AEN-203** (MUST) The same G(d) feeds the audibility (AEN-140) and the voice volume (AEN-214).

**AEN-204** (MUST) **Stereo output.** When the mastering voice has two channels, the engine computes each real mono positioned voice's output matrix with the constant-power pan of [positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md) R-AUD-205: with u the unit vector from the listener to the emitter and r the listener's right vector (AEN-209), x = u · r, p = (x + 1) ÷ 2, left = cos(p × π ÷ 2) and right = sin(p × π ÷ 2); an emitter at the listener's position is centred (x = 0). A two-dimensional mono voice takes the centred value on both channels; a two-dimensional stereo voice maps left to left and right to right, each at the centred value, so a stereo member made of a duplicated mono signal sounds like that mono member.

**AEN-205** (MUST) **Swap left and right.** With the setting on, r is reversed for the stereo pan, and the output matrix's left and right channels are exchanged on every other layout (front, side and back pairs); two-dimensional voices are unaffected because they are centred (R-META-435).

**AEN-206** (MUST) **Other layouts.** For a mastering voice with more than two channels, the engine calls `X3DAudioCalculate` with the emitter's position and velocity, a single-channel emitter, a volume curve that is constant at 1, so that the matrix carries only panning, and the listener of AEN-209; the distance gain G is applied as the voice volume. Two-dimensional voices feed the front-left and front-right channels at the centred value.

**AEN-207** (MUST) **Doppler, distance filtering and distance reverb.** X3DAudio's Doppler factor, direct-path low-pass coefficient and reverb level are scaled by the event's factors (AEN-062), whose defaults `aen.spatial.doppler_factor_default`, `aen.spatial.filter_factor_default` and `aen.spatial.reverb_factor_default` are 0, so with every default the frequency ratio, the voice filter and the reverb send are neutral, and the mix has none of them (R-AUD-201). On stereo output the engine requests these values from `X3DAudioCalculate` only for events with a non-zero factor.

**AEN-208** (MUST) Two-dimensional instances, and music, bypass every spatial calculation.

**AEN-209** (MUST) **Listener.** The listener is the camera of the latest `SetListener` (R-AUD-200, DEC-566): its position, its front and up vectors and its velocity × `aen.listener.velocity_scale`. r = Normalize(front × up) in the right-handed world frame (ADR-016); z is negated once, at the boundary, for X3DAudio's left-handed convention. A listener command whose front or up vector has zero length, or whose front and up are parallel, is rejected, and the previous orientation is kept (ADR-027).

**AEN-210** (MUST) Every real positioned voice is recalculated at every update, whatever its movement (ADR-027, R-AUD-208); a value may be skipped only when it is bit-identical to the value already set.

**AEN-211** (MUST) The engine accepts every minimum distance of the design's range `ref.aud.layer.min_distance_range`; the content build rejects other values.

### 4.15 Volume law and gain stages

**AEN-214** (MUST) A real voice's volume is the amplitude of its envelope level (AEN-166) + 20 log₁₀ G(d) (0 for two-dimensional voices), converted with `XAudio2DecibelsToAmplitudeRatio`; a total at or below `ref.aud.mix.floor_db` is set to that floor (R-AUD-224, R-AUD-225).

**AEN-215** (MUST) **Bus and master gains.** A player bus's gain is its default level `ref.aud.bus.<bus>.level` (R-AUD-221) − `ref.aud.mix.master_step_db` × (`ref.meta.options.sound.volume_max` − its volume step), plus its ducking (AEN-216); the pre-master gain is −`ref.aud.mix.master_step_db` × (`ref.meta.options.sound.volume_max` − the master step) (R-AUD-225). No stage goes below `ref.aud.mix.floor_db`. A change takes effect at the next update, ramped over one update period (AEN-149), and each player setting changes exactly one bus gain; the bus volumes are those of DEC-044 and DEC-570.

**AEN-216** (MUST) **Ducking.** When an instance on the announcer bus starts, the world bus is ducked by `ref.aud.bus.announcer.duck_world_db` until `ref.aud.bus.announcer.duck_time` after the latest such start; ducks do not add up. The duck enters and leaves over `aen.mix.duck_ramp`.

### 4.16 Buses, routing and effects

**AEN-220** (MUST) **Bus graph** (ADR-024, DEC-570). The graph is: the mastering voice; under it the **pre-master** submix, which applies the pre-master gain and the built-in `FXMasteringLimiter` (release `aen.mix.limiter.release`, loudness `aen.mix.limiter.loudness`), set so that a signal below full scale passes unchanged; under it the six player buses of R-AUD-221, UI, announcer and stings, voice, world, ambience and music, each a submix; under the player buses the effect buses of AEN-223, each feeding the player bus of the sounds routed to it. Every submix has the mastering voice's channel count and sample rate, and processing stages order effect buses before player buses before the pre-master. Every source voice outputs to its event's bus (R-AUD-222) and, through sends, to effect buses.

**AEN-221** (MUST) **Effects switch** (DEC-567, DEC-752, R-AUD-076). With the switch off, which is the default (`aen.settings.effects_default`), every source voice has a send level of 0 and a direct level of 1 and every effect bus's chain is disabled, so no effect processes any signal. Switching takes effect at the next update for every live instance, with no restart; switching off leaves no processing on any voice or bus (DEC-752).

**AEN-222** (MUST) Until DEC-567 is decided, the mix is heard dry (R-AUD-076): the effect bindings of the data are cooked and validated, the switch stays off, and the limiter is the only processing of the graph.

**AEN-223** (MUST) **Effect buses.** The content build creates one effect bus for each distinct combination of a parent player bus and a chain of effect kinds with fixed values of their non-mix parameters among the effect bindings of the data (R-AUD-081). A binding whose non-mix parameter is random or linked (the gargle of `transport.spawn`, R-AUD-082) gets its own effect bus per parent bus, whose parameters follow the highest-ranked real instance routed to it at each update.

**AEN-224** (MUST) **Routing.** A chain of one **send** kind (AEN-070) runs 100 % wet on its bus, and the voice's direct level is 1 − w and its send level w, where w = the binding's wet/dry mix ÷ 100, evaluated per instance (AEN-188); because the send kinds are linear, this equals the mix x × (1 − w) + effect × w that the effect's wet/dry mix defines (R-AUD-079). A chain of one **insert** kind, or of several kinds, takes the whole signal: the direct level is 0, the send level 1, and the send kinds in a multi-kind chain apply their own wet/dry mix inside the effect. The effect bus's chain holds the own-algorithm kinds in kind order, then the standard kinds in kind order (R-AUD-080).

**AEN-225** (MUST) Every Species XAPO processes 32-bit float at its bus's actual sample rate, with its own state per channel, and computes every time constant and coefficient from that rate (ADR-024). Full scale ±1.0 corresponds to `aen.fx.full_scale` on the 16-bit scale on which the effect constants of R-AUD-079 and of this section are stated. `StereoEchoFx` applies the left delay to the left and centre channels and the right delay to the right channels.

**AEN-226** (MUST) At every update each effect bus's parameters are evaluated (AEN-188) and set within the update's operation set; integer parameters are truncated toward zero (AEN-070).

**AEN-228** (MUST) Because a bus effect's tail is produced by the effect, a voice's release envelope does not shape it, so the long releases that let an echo tail ring (R-AUD-074) hold a voice without shaping the tail. Sounds are authored for the effect model that DEC-567 chooses.

The own-algorithm effects (R-AUD-079) compute, per sample x on the 16-bit scale (the float sample × `aen.fx.full_scale`), with rate = the bus sample rate:

**AEN-230** (MUST) **Resonant low-pass** (`ResonantLowPassFx`). From the parameters p0, p1, p2: f = e^(p0 ÷ `aen.fx.lowpass.cutoff_divisor` + `aen.fx.lowpass.cutoff_offset`) Hz; Q = max(e^(p1 ÷ `aen.fx.lowpass.q_divisor`) − 1, `aen.fx.lowpass.q_floor`); gain = p2. Then ratio = min(f ÷ rate, `aen.fx.lowpass.max_ratio`), ω = 2π × ratio, α = sin ω ÷ (2Q); a0 = a2 = (1 − cos ω) ÷ 2 ÷ (1 + α), a1 = (1 − cos ω) ÷ (1 + α), b1 = −2 cos ω ÷ (1 + α), b2 = (1 − α) ÷ (1 + α). Per sample y = a0 x + a1 x₁ + a2 x₂ − b1 y₁ − b2 y₂, and the output is round(clamp(y × gain, ±`aen.fx.lowpass.clamp`)); after each block `aen.fx.denormal_offset` is added to y₁. Before its first parameters the filter uses `aen.fx.lowpass.initial_cutoff`, Q 1 and gain 1.

**AEN-231** (MUST) **Bit crusher** (`BitCrusherFx`). step = 65536 ÷ 2^b for b bits; output = step × trunc(x ÷ step), clamped to ±`aen.fx.bitcrusher.clamp`.

**AEN-232** (MUST) **Gargle** (`GargleFx`). A phase φ in [0, 2) advances by 2 × rate_hz ÷ rate per sample and wraps at 2; the triangle is tri = φ for φ < 1 and 2 − φ otherwise. Wave shape 0: output = x × (1 − w) + x × tri × w, w = mix ÷ 100 (as a send, the bus outputs x × tri). Wave shape 1: output = x for φ < 1 and 0 otherwise; the mix is ignored. The wave shape is an integer parameter.

**AEN-233** (MUST) **Echo** (`EchoFx`). A float delay line of `aen.fx.echo.buffer_seconds` × rate samples; D = delay_ms × rate ÷ 1000 samples, truncated. Per sample: output = clamp(x × (1 − w) + line[j] × w, ±`aen.fx.echo.clamp`) with w = mix ÷ 100 (as a send, the bus outputs line[j]); then line[j] := line[j] × attenuation ÷ 100 + x; j advances and wraps at D, not at the line length. When D grows, the newly exposed part of the line is set to 0.

**AEN-234** (MUST) **Simple reverb** (`SimpleReverbFx`). A history h of `aen.fx.reverb.history_seconds` × rate samples and six taps at the delays `aen.fx.reverb.tap_delays_ms` with the gains `aen.fx.reverb.tap_gains`: y = `aen.fx.reverb.dry` × x + `aen.fx.reverb.wet` × Σ gainᵢ × h[n − delayᵢ], clamped to ±`aen.fx.reverb.clamp` and written back to h[n]. The mix parameter is ignored. The taps are summed in float, with no truncation of the products (ADR-024).

**AEN-235** (MUST) Each standard kind realised by a Species XAPO (AEN-070: `StereoEchoFx` and `StudioReverbFx`) implements its parameter set of AEN-070.

### 4.17 Sample residency

**AEN-243** (MUST) **Residency** (ADR-025). Resident groups are cooked to PCM and held in banks (AEN-074); a resident member is never decoded at play time and starts without delay. Streamed groups are read from the packs buffer by buffer and decoded by XAudio2 as they play (§4.18); no streamed member is ever held whole in memory. Nothing is evicted while its bank's scope lasts; a bank is released with its scope. Each bank has a hard budget, and a bank whose resident groups exceed it is a content-build error: `aen.memory.interface_bank_budget` for the interface bank and `aen.memory.match_bank_budget` for the match bank, with `aen.memory.stream_budget` for every stream buffer and stream head together.

**AEN-244** (MUST) `LoadBank` asks the asset system to read the bank's groups from the pack files (ADR-058) asynchronously, through its content reader ([asset pipeline and resources](../asset_pipeline_and_resources.md) AST-114); the bank is resident when every group and stream head is in memory (AEN-104). `ReleaseBank` first retires every live instance whose group would be freed, at once, and then frees the groups whose reference count falls to 0.

**AEN-245** (MUST) Group residency is counted by reference across banks, so a group of two resident banks is loaded once and freed with the last of them.

**AEN-246** (MUST) Banks hold their groups keyed by asset id, mono and stereo members alike (AEN-072); the memory per bank and for streams is reported in the memory summary of AEN-324 and in the development overlay (AEN-323).

**AEN-247** (MUST) A trigger whose group is not resident is dropped (AEN-111); a member missing from a resident bank is a diagnostic and its layer is silent (R-AUD-046).

### 4.18 Decoding and streaming

**AEN-251** (MUST) **Decoding.** The engine contains no decoder (ADR-025, `AGENTS.md` R14). A resident member is 16-bit PCM in its bank and is submitted as it is; a streamed member is a Species stream of MS-ADPCM blocks (AEN-072) that XAudio2 decodes inside its processing, block by block as the voice plays. The content build's audio cooker reads every source and writes both forms ([asset pipeline and resources](../asset_pipeline_and_resources.md) AST-079); a source in another format, at another rate or with no sample frames is a build error there, so no member reaches the engine undecodable. A stream that is corrupt or truncated at run time retires its instance (AEN-256); nothing in decoding stops the program (DEC-015).

**AEN-252** (MUST) **Species streams** (ADR-025). A real streamed instance owns an `AudioStream`: a ring of `aen.stream.blocks_per_stream` buffers, each holding `aen.stream.block_frames` frames rounded up to whole MS-ADPCM blocks of its group, which the stream task refills one buffer at a time from the pack when the voice's buffer-end notice frees one (AEN-025), and which it submits in order to the instance's MS-ADPCM source voice; XAudio2 decodes them (ADR-025). At most `aen.stream.max_streams` streamed instances are real at once; a streamed instance that ranks for a voice beyond that stays virtual and is counted. A virtual streamed instance holds no stream and keeps only its play position.

**AEN-253** (MUST) **Stream heads.** The first buffer of every member of a streamed group is read when its bank loads and kept with the bank (AEN-074), so a stream that starts at position 0 starts without waiting for the stream task.

**AEN-254** (MUST) A streamed instance that becomes real at a position p > 0 starts at the start of the MS-ADPCM block that holds p, frame ⌊p ÷ `aen.stream.samples_per_block`⌋ × `aen.stream.samples_per_block`, and its play position is set back to that frame, because seeks work on whole blocks (ADR-025); it stays virtual until its first buffer is read, and a start delayed by this is counted as late.

**AEN-255** (MUST) Loop start and loop end lie on block boundaries, which the content build checks (ADR-025): at the loop end the stream continues from the loop start without a gap; a `loop` layer's loop delay inserts silence as for resident members (AEN-176).

**AEN-256** (MUST) A stream with a corrupt header, a truncated block or a missing entry retires the instance with a diagnostic; nothing in streaming stops the program (DEC-015).

**AEN-257** (MUST NOT) No stream read runs on a game thread or inside an XAudio2 callback; decoding runs only inside XAudio2 (ADR-025, ADR-026).

### 4.19 Device loss and restart

**AEN-259** (MUST) On `RestartDevice`, on `IXAudio2EngineCallback::OnCriticalError` and on `XAUDIO2_E_DEVICE_INVALIDATED`, the engine rebuilds the graph and the voice pools on the current default device, resubmits every real instance from its stored play position with its current settings, and continues: loops and one-shots continue where they were, and no instance restarts from its beginning (ADR-022). If the rebuild fails, the engine runs on the silent device (AEN-321) and tries again at the next `RestartDevice` or device-arrival notice.

**AEN-260** (MUST) **Default-device change.** When the platform layer reports that the default output device changed or that a device arrived ([platform layer](../platform_layer.md) PLT-103), the engine's endpoint listener issues `RestartDevice`, and the engine recreates its mastering voice and graph on the new default device, in that device's format, by AEN-259 (ADR-022). The other endpoint changes (a device removed while it is not the default, a state change) leave the graph as it is.

**AEN-261** (MUST) Applying the sound options sends the changed settings as commands: the volumes act at once through `SetMasterVolume` and `SetBusVolume` (R-META-433, R-META-434), and swap left and right through `SetSwapStereo` (R-META-435). No apply issues `RestartDevice`, so applying the sound options never restarts or cuts a sound (R-META-437).

## 5 Concurrency and performance budget

The threads, the budgets and the memory figures are [README](README.md) §5.

## 6 Failure modes, diagnostics and telemetry

The failure table, the silent device and the diagnostics are [README](README.md) §6.

## 7 Acceptance tests

The tests of this document are in [README](README.md) §7: T19 to T26 and T28.

## 8 Open decisions

The decisions and architecture records are [README](README.md) §8.1.
