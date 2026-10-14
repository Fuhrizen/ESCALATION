# Basic audio

The native client has eight short, original synthesized effects: selection, movement order, successful action, rejected action, research/construction completion, missile launch, impact and UI click. These are understated cues rather than spoken acknowledgements or continuous battle ambience. The uploaded pack contains anonymously numbered recordings; those recordings are not used without identifying their content.

`Audio` mixes up to twelve voices at 24 kHz, mono, signed 16-bit PCM. A worker handles playback so the simulation and interface do not wait for an audio buffer. Volume and mute take effect on the next 25 ms block. Events are rate-limited individually (55 ms for interface actions, 150 ms for launches/impacts), and a bounded queue prevents bursts from accumulating. Silent/muted events are discarded.

Windows uses the operating system's `waveOut` API (`winmm`). Linux dynamically loads `libasound.so.2` and uses the default ALSA playback device; no ALSA development headers or additional download are required. Missing library/device or playback errors disable audio gracefully, so headless runs still work. Windows buffer completion has a buffer-duration-plus-two-second timeout and resets playback on failure instead of waiting indefinitely. Device availability is reported asynchronously by `available()` after `start()` schedules the playback thread. Device recovery/reselection requires restarting audio/the application; there is no hot-plug menu yet.

`tools/prepare_audio.py` exports the same original cues as editable/listenable WAV files. These are previews/source assets; runtime synthesis does not depend on their paths and does not currently load replacements. No music, voice acting or third-party recording is bundled.

Integration uses `play(SoundCue::...)` once for accepted player actions, newly completed projects, new missile launches and resolved impacts. Per-frame hover or continuous held controls should not play sounds. Selection and order feedback should correspond to actual state changes rather than input alone.

Backend references: [ALSA PCM interface](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html) and [Microsoft waveOut](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutopen).

Validation: strict Linux compilation and finite/bounded cue sample checks passed. A temporary ALSA `null` default device exercised initialization, availability, all eight cues, volume/mute, shutdown and restart successfully. This checks backend flow without claiming speaker listening or Windows execution.
