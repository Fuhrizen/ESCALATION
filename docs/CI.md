# Continuous validation

[Validation runs](https://github.com/Fuhrizen/ESCALATION/actions/workflows/validation.yml)

The Validation workflow runs on code pushes to main and pull requests targeting main. Documentation-only changes skip it. The Actions page also provides Run workflow for a complete manual check and compressed render images.

Two standard hosted runners build the actual Release client: Ubuntu 24.04 with GCC and Windows 2022 with MSVC. Both run simulation/save migration, tactical combat, camera and native UI input tests. Linux additionally runs the package orchestration contract tests, real software-OpenGL/EGL smoke rendering (including the globe), and the frontline benchmark. Both install the game into a package, temporarily hide source data, and run the relocated headless executable to verify data lookup.

Read the job summary for per-test results and durations. Benchmark timings are diagnostics; shared runner hardware varies and there is no hard timing threshold. They are not evidence of full-world-war scalability. Logs and JUnit reports are downloadable for one day. Linux screenshots are converted from PPM to PNG and retained only on failure or an explicit manual run. Ordinary CI does not upload executables or release packages. The existing Major release packages workflow remains separate.

## Runner and storage budget

As checked on 14 October 2026, standard GitHub-hosted runners are free for public repositories; the private-repository 2,000-minute allowance does not constrain this public repository. GitHub Free lists 20 concurrent standard jobs, 500 MB artifact storage and 10 GB repository cache storage. Account-specific limits and repository policies still apply. Sources:

- https://docs.github.com/en/billing/concepts/product-billing/github-actions
- https://docs.github.com/en/actions/reference/limits

This workflow uses at most two jobs per run and two compiler workers per job, with a 20-minute job timeout and a three-minute per-test timeout. A newer update to the same branch/PR cancels an older run. Only the pinned GLFW/ImGui source checkouts are cached, keyed by OS and CMakeLists.txt; build outputs and world data are not cached. Reports expire after one day and raw PPMs/binaries are never uploaded. No larger runners, self-hosted machines, AI agents, tokens or paid services are needed.

Actions are pinned to verified commit SHAs. Jobs have contents-read permission and checkouts do not preserve credentials. Fork pull requests use the normal pull_request event; their code receives no repository secrets. Some first-time contributor runs may require GitHub's normal approval.

## Development workflow

Use a narrow local compile/test for the code being edited, then push the completed commit and read CI. Prefer fixing an actual failed job and its logs over repeatedly rebuilding all platforms locally or spending agent turns on the same checks. CI supplies automatic validation, not a substitute for visual review or gameplay design judgement.

The single-file local verifier now includes all core translation units, including the technology catalogue and persistent territory implementation. Container/Bazzite integration still requires running on a host with the relevant container engine; the orchestration contract tests mock that engine.
