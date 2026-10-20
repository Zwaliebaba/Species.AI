---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; workflow decided by the owner's AGENTS.md (O21)
---

# Continuous integration: the committed `build.yml` workflow, with further jobs added by amendment and guarded the same way

## Context and Problem Statement

Every change to Species code or content must be built, checked and tested before it is merged. The repository holds its CI workflow, `.github/workflows/build.yml`, which the engineering rules describe (`AGENTS.md` §6): a Windows job and two Linux jobs (formatting and the design checks), every step that has something to run blocks, and a step whose input does not exist yet is skipped and says so, never faked. CI builds `Debug|x64` only; the static check of the two configurations stands in for a Release build ([ADR-054](ADR-054-build-system-msbuild-solution.md)). Long-running checks (a soak of many matches, fuzzing, GPU validation, performance runs) and the design checks are needed too, without slowing every change and without weakening the guard rule. The CI service is GitHub Actions, as the workflow file implies; agent hosting beyond the hosted runners is an infrastructure choice outside this ADR.

What does Species' continuous integration run, and how does it grow?

## Decision Drivers

* CI runs what developers run: the solution build, the test suites and the checkers (`AGENTS.md` §3).
* Every step that has something to run blocks; nothing is `continue-on-error`.
* A step whose input does not exist yet is skipped, not faked, and starts gating the moment its input lands.
* Toolchain versions that can turn a build red on their own are pinned; the rest is printed.
* Crashes, content errors and performance regressions are found with artefacts.

## Considered Options

* The committed workflow as the baseline, with further jobs added by amendment of this record and guarded the same way
* A two-tier design written in advance (per-change and nightly jobs) for a tree that has no code yet
* Manual release testing only

## Decision Outcome

Chosen option: "The committed workflow as the baseline, with further jobs added by amendment". Accepted: the workflow is decided by the owner's `AGENTS.md` (O21).

* **Triggers:** pushes and pull requests to `main`, and manual runs; a newer run of the same ref cancels an older one.
* **Windows job** (`Debug|x64`, on the hosted Windows runner):
  * print the toolchain it got: the runner image, Visual Studio, MSBuild, clang-tidy and Python;
  * locate the solution: exactly one `.slnx` or `.sln` at the repository root, or none while the tree is empty; two fail the job;
  * check the build shape and the project registries with `Build\CheckProjectFiles.py`, guarded on the script;
  * restore the allow-listed NuGet packages ([ADR-055](ADR-055-dependencies-three-nuget-packages.md)) with `/t:Restore /p:RestorePackagesConfig=true` before the build, guarded on the solution and on a `packages.config` anywhere in the tree;
  * check the generated schema code with `Build\CheckGeneratedSchemas.py`, which builds the generator through the solution, regenerates the code into a temporary tree and fails on any difference ([ADR-062](ADR-062-authoring-syntax-and-schema-language.md)), guarded on the script;
  * build `Debug|x64` through the solution, every project and the tool projects among them ([ADR-054](ADR-054-build-system-msbuild-solution.md)), with MSBuild warnings as errors, passing the workflow's run number as the build number of the build identity (`/p:EngineBuildNumber=${{ github.run_number }}`, [ADR-057](ADR-057-version-identity.md)), guarded on the solution;
  * run the test suites: one suite per `*Tests.vcxproj`, each of which must have produced its DLL, run by `vstest.console.exe` with a `.trx` log ([ADR-069](ADR-069-test-framework.md)), guarded on the solution;
  * run clang-tidy over the tree with `Build\RunClangTidy.py`, guarded on the script; clang-tidy is pinned (22.1.8, installed from pip) so that a runner-image change cannot turn the build red;
  * upload the build log and the test results.
* **Formatting job** (Linux): check formatting with `Build\CheckFormat.py` on clang-format 18 (installed from the Ubuntu packages, 18.1.3), guarded on the script.
* **Design job** (Linux): install the pinned Mermaid checker (`npm ci --prefix Tools/DesignCheck`, then Playwright's Chromium) and run `python3 Tools/DesignCheck/CheckDesign.py Design`, both guarded on `Tools/DesignCheck/CheckDesign.py` and `Design/README.md` (`AGENTS.md` §6).
* **Release is not built in CI.** Before a release, whoever ships builds `Release|x64` and runs the release checks of the other records (mitigation flags, symbols, version resource, no development or loose-file code, install contents).
* **Further jobs** are added by amendment of this record and of the workflow, each guarded on the file it needs, in this order of need:
  * the content build of every edition with all validators ([ADR-059](ADR-059-content-build-and-validators.md));
  * a scheduled job for the headless bot soak (the server plays matches between bots on every mode and map at maximum speed and reports crashes, assertion failures, simulation update-time percentiles and peak population per match), the reproducibility check of the content build, and performance tests against component budgets;
  * on an agent with a GPU, golden-image tests and a client run with the Direct3D 12 debug layer and GPU-based validation.
* **Artefacts:** build logs and test results now; soak reports, performance results and crash dumps from test machines ([ADR-065](ADR-065-crash-reporting.md)) with the jobs that produce them.
* **Replays:** if the design phase gives Species reproducible match replays, recorded matches become regression tests that compare end-of-match state hashes, in a job added the same way.

### Consequences

* Good, because the workflow is honest about an empty tree: every gate starts blocking the moment its input exists, with no edit to the workflow.
* Good, because the clang-tidy and clang-format pins stop a runner-image change from failing unchanged code.
* Bad, because Release is never compiled in CI; a defect that appears only under optimisation is found by whoever builds Release.
* Bad, because the soak, golden-image and performance checks do not exist until their jobs are added, coverage-guided fuzzing needs an owner decision first ([ADR-069](ADR-069-test-framework.md)), and the GPU checks need an agent the hosted runners do not provide.

### Confirmation

* The workflow has no `continue-on-error` step; every skipped step names its missing input in the log.
* The merge rule requires the Windows job and both Linux jobs to pass.
* Each amendment that adds a job states its guard and its input, and removes the guard once the input is permanently present.

## Pros and Cons of the Options

### The committed workflow, grown by amendment

* Good, because CI matches the tree as it is and as `AGENTS.md` describes it.
* Neutral, because long-running checks wait for their jobs.

### A two-tier design written in advance

* Good, because the target pipeline is visible from the start.
* Bad, because jobs without inputs would be faked or permanently skipped, which `AGENTS.md` §6 forbids, and they describe configurations the build no longer has.

### Manual release testing

* Bad, because regressions are found late and without reproducible artefacts.

## More Information

* Rules: `AGENTS.md` §6 (what CI runs, guards, CI does not build Release) and §3 (build and test commands).
* Source: `.github/workflows/build.yml` (the committed workflow; the clang-tidy and clang-format pins and their reasons are in it).
* Components: diagnostics, test and performance (`DBG`), build tooling and distribution (`BLD`).
* **Amendment 2026-10-17.** Job presets carry the product suffix of ADR-054 (`<os>-<compiler>-<configuration>-<product>`, the configuration Debug or Release, BLD-006); the build tooling component lists the jobs and their workflow presets.
* **Amendment 2026-10-17 (AGENTS.md, O21).** CI is the committed `build.yml`: a Windows job (build shape check, `Debug|x64` build, test suites, clang-tidy) and a Linux job (pinned clang-format); Release is not built in CI; the design checks, content build, nightly soak, fuzzing, GPU and performance jobs are added by amendment, guarded the same way. The preset-based jobs of the earlier amendment above, the analysis builds and the `import std` build are removed.
* **Amendment 2026-10-17 (`build.yml`, O27).** The committed workflow already holds the NuGet restore step (guarded on the solution and on a `packages.config`) and the design-checks job (guarded on `Tools/DesignCheck/CheckDesign.py` and `Design/README.md`), so neither is a future amendment any more: the decision lists the restore step in the Windows job and the design job beside the formatting job, and the list of further jobs starts with the content build.
* **Amendment 2026-10-17 (`build.yml`, O28).** The committed workflow also checks the generated schema code before the build (`Build\CheckGeneratedSchemas.py`, guarded on the script; ADR-062), and its build passes the run number as `EngineBuildNumber` (ADR-057; build tooling table 2.1-B); a local build leaves it at 0.
* **Amendment 2026-10-18 (owner decision O29, O27).** The tool projects are projects of the one solution, so the Windows job's build compiles them on every run and no separate tools job is added; the generated-code check builds the generator through the solution. The fuzz harnesses run in the test step as test methods of the `<Project>Tests` projects ([ADR-069](ADR-069-test-framework.md)), and the scheduled fuzzing job is removed from the further jobs, because a coverage-guided fuzzing project needs an owner decision first.
* **Amendment 2026-10-20 (M1 task T11, [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md)).** The Windows job's build runs the content build as the post-build step of its tool project. After the build, the step "Check the content build" (`Build\CheckContentBuild.py`) runs the tool twice and compares the two folders byte for byte, then runs its 23 fixtures, each of which must fail and name its file and fault.
* **Amendment 2026-10-20 (M1 task T10, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** After the content check, the step "Check the performance script" runs `Build\RunPerformance.py --self-test` on the `Debug|x64` performance tool: every measurement scene for a few ticks, and the record's rules on fixtures. CI times nothing; the owner runs the performance job on the reference test machine (O69).
