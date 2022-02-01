# GitHub Actions Workflows

**PLEASE READ THROUGH THIS FILE COMPLETELY BEFORE MAKING ANY CHANGES TO THE MAIN.YML FILE**

## Prerequisites

- Understanding of the [GDev System](../../dev_tools/gdev/README.md)
- Understanding of [GitHub Actions](https://docs.github.com/en/actions)
- A good [cheat sheet](https://github.github.io/actions-cheat-sheet/actions-cheat-sheet.pdf) on GitHub Actions

## Design Goals

- Simplicity - As little "magic" as possible.
- Reproducibility - With small modifications where needed, most steps can be executed on a developer machine.
- Readability - Clear scripts and workflow files.

## Design

The main workflow, `main.yml`, is constructed to do as much as possible in the early stages with as little lifting as possible.
To that extent, the `Third-Party` job and the `Lint` job are at the front of the workflow, reducing friction for the rest of the workflow.
From there, the build jobs are then triggered along with any test jobs that are required.
Once one of the build jobs (currently the `SDK` job) produces a package artifact, then the install jobs the use that artifact can be executed.
Where possible, jobs should be run in parallel to reduce the time-to-results.
However, that effort must be balanced against the costs required.

For example, executing the `Third-Party`, `Lint`, and `Core` jobs before any other job will provide signifant coverage for the other builds, reducing the chances of a failure in one of those builds.
That statement is partially based on the compilation of all third-party packages and the linting of the entire project before any part of the main project is touched.
The other half of that statement is based on experience and observation that most of the failures occur during the build phase or the unit test phase, both which are encapsulated in the `Core` build job.

## Workflow Triggers

Based on experience, there are three main usage patterns for executing the main workflow:

1. A manually triggered build against a non-`master` branch, to verify that a change works properly and does not have unintended side effects.
1. As part of a *Pull Request* against `master`, to assure the reviewer that (at the very least) basic steps have been undertaken to verify that the code is correct.
1. As part of a *Push* against `master`, to verify that the change works properly once combined with everyone else's work.

These usage patterns are supported in the `main.yml` file by the trigger specification at the top of the workflow:

```YAML
on:
  push:
    branches:
      - master
  pull_request:
    branches:
      - master
  workflow_dispatch:
```

and this `if` conditional at the start of some of the jobs:

```yaml
    if: github.event_name != 'pull_request'
```

### Manually Triggered Workflows

This usage pattern is supported by the `workflow_dispatch` trigger.
When the workflow is started, a specific branch can be specified in the GitHub UI.
As the main focus of this usage pattern is to verify a code change, the full set of jobs are executed on the specified branch.

To enable the developer to have more confidence in their proposed changes before a *Pull Request*, they can decide to use one of the manually executed workflows to verify their changes.
The currently available manual workflows are:

- `Main` - Same workflow that is used when changes are pushed to the `master` branch
- `Core` - Specifically the `Lint` job and the `Core` and `Debug_Core` jobs
- `Core and SDK Jobs` - Reduced set of jobs, only those necessary to build the `Core` and `SDK` jobs and their variations
- `LLVM Tests` - Only the `LLVM_Tests` job
  - Note that inside of that job, the required Core and SDK components are built as part of the `LLVM_Tests` job

Also note that to maintain integrity between these "child" workflows and the main workflow, there is a Pre-Commit hook that verifies those child workflows.
While developers are encouraged to come up with their own workflows to test given scenarios, there is no provision for a `scratch` directory for workflows.
If a situation occurs where a developer is frequently using their own custom workflow in their own branch prior to submitting a *Pull Request*, please bring it up on the main channel to talk about it with the team.

### Pull Request

This usage pattern is supported by the `pull_request` trigger, its `branches` specification, and the `if` condition.
To keep things concise, a shortened list of jobs is executed against a *Pull Request*.
Those jobs are:

- `Lint`
- `Third-Party`
- `Core`
- `Final`

Jobs that are not in that short list include the above `if` conditional at the start of their jobs and are not executed.

### Merging a Pull Request

This usage pattern is a usage pattern to verify that the reviewed changes are still viable after those changes are merged into the `master` branch.
Like the scenario for **Before Full Request**, this usage pattern is about verification, executing the full set of jobs to achieve that goal.

#### References

- [Manually running a workflow](https://docs.github.com/en/actions/managing-workflow-runs/manually-running-a-workflow)

## Information Relevant to All(Most Jobs and Their Steps)

The `Lint` job is special, and is covered in its own section below.
For the other sections, there is a lot of overlap.

### Dependent Jobs

With the exception of the `Third-Party` and the `Lint` jobs, every other job has dependencies on other jobs within the workflow.
While it is possible to kick off every job at once, it is important to structure the workflow so that new jobs are started only once we have a decent belief that the job will succeed.

```YAML
  needs:
    - Lint
    - Third-Party
```

A good example of this is the `Debug_Core` job.
Added to provide additional coverage in a `Debug` environment, any non-Debug concern of this job is met by its dependency on the `Core` job.
Likewise, the `Core` job is dependent on the `Third-Party` and the `Lint` jobs for similar reasons.
If the `Third-Party` job fails to build the base image, the `Core` image has no chance of succeeding.
If the `Lint` job fails, there is enough doubt about whether the project code is in the right form to not build anything else.

Please remember, we can always throw faster machines and improve workflows, but we have to be smart about how we do that.

### Build System

Each of our jobs is built on a standard Ubuntu:20.04 system.
This was done as our standard development environment is a standard Ubuntu:20.04 system, and I did not want people to have to learn the peculiarties of a new version of linux.
To keep things simple, any specialization of jobs for different environments is done within the docker builds.

```YAML
    runs-on: ubuntu-20.04
```

### Environment Variables

These are set in each job to ensure a stable environment is available at the workflow level:

```YAML
env:
  GAIA_VERSION: 0.3.3
  SSH_AUTH_SOCK: /tmp/ssh_agent.sock
  DEV_IMAGE: ghcr.io/gaia-platform/dev-base:latest
```

and the job level:

```YAML
env:
  GAIA_REPO: ${{ github.workspace }}
```

| Name | Description |
| --- | --- |
| `GAIA_REPO` | where the workspace is within the workspace provided by GHA |
| `GAIA_VERSION` | version of the product being built |
| `SSH_AUTH_SOCK` | needed for docker related commands issued within the scripts |
| `DEV_IMAGE` | image to use as a base/cache for the other images to be built |

### Steps Prefix

For every job, two prerequisites are required: the repository and Python3.8 being installed to work against that repository.
Therefore, the `steps` section for each job always starts with the following prefix:

```YAML
    steps:
      - name: Checkout Repository
        uses: actions/checkout@master

      - name: Setup Python 3.8
        uses: actions/setup-python@v2
        with:
          python-version: 3.8
```

These two actions are very important, as they set up to foundation of each job.
The first action does a `git checkout` of the code belonging to the branch that was triggered on.
The checkout is performed to the `$GAIA_REPO` or `${{ github.workspace }}` directory.
The second action ensures that the current version of Python 3.8 is installed to the image.
This is required as the `gdev.sh` infrastructure is written in Python, and has been fully tested against Python 3.8.

### Steps Suffix

Most of the jobs have a step similar to:

```YAML
      - name: Upload Output Files
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: ${{github.job}} Output Files
          path: |
            ${{ github.workspace }}/build/output

```

that is used to make any files produced by the job available for later inspection.
To keep things as simple as possible for builds, exposing files for Build Jobs is done by copying files or directories to `${{ github.workspace }}/build/output`.
Because there is a wider range of possibilities with Test Jobs, the `path` to export is specifically set for each Test Job.
However, if possible, the `${{ github.workspace }}/production/tests/results` directory is used.

## `Third-Party` Job

This job is a special job because it creates a new image for the dev-base.
There are multiple reasons for doing this, the most important being disk space and caching.

### Reasoning

The first reason is one of the most important ones.
With the manner in which the current docker image files are created, they take up a lot of disk space to create.
While the end image is only in the 2-3 GB range, depending on the options, that same image can take between 36GB and 50GB of disk space to create.
As the standard GitHub Action build machines only have 40GB and we should not be wastedful with the disk space on our standard developer machines, it made sense to capture this "expense" in a separate job.

The second reason is also an important reason, one that should not be dismissed.
One of the reasons why Docker and other image/container systems are popular is because they have become very good at caching contents.
But that caching comes at the price of organization and understanding how caching works.

For Docker, caching is done on a per-instruction basis within the Dockerfile being built.
Each instruction in the file produces a single layer that is part of the Docker image that is generated.
This layer describes the differences between the current layer and the next layer to be added on top of it.
Now, Docker is very good at figuring out if a given instruction in the Dockerfile has the same information as in a previous build step.
However, that caching is like the game of [Jenga](https://en.wikipedia.org/wiki/Jenga) played with a tower that is one-brick wide.
Once an instruction dictates the creation of a new layer instead of reusing a cached layer, the cache cannot be used for any new layers.
Note that there are some *weird* exceptions to that rule, but unless you are very good at understanding Docker, it is best not to think of them.

Why is this important?

With very few exceptions, the subprojects and source code that is built in the Dockerfiles before the line:

```Dockerfile
# GenRunDockerfile(production)
```

are static.
There are no special tags that have to be interpretted in those subprojects, specifying special behavior under different build configurations.
For the two reasons mentioned above, it just makes sense to pay this price once, and leverage it from there.

### Creating the Base Image

Because of the previously stated reasons, the Dockerfile that is generated by the gdev framework is slightly modified:

```sh
$GAIA_REPO/dev_tools/gdev/gdev.sh dockerfile > $GAIA_REPO/production/raw_dockerfile
$GAIA_REPO/dev_tools/github-actions/copy_until_line.py --input-file $GAIA_REPO/production/raw_dockerfile --output-file $GAIA_REPO/production/dockerfile --stop-line "# GenRunDockerfile(production)"
```

This script copies the `dockerfile` generated by `gdev` up to the line specified in the earlier section.
With a few exceptions that have already been mitigated, each generated Dockerfile before that line is exactly the same.
As such, this allows the script to dynamically generate the base image's dockerfile.

Once the base image has been created, it is upload to GitHub's Image directory as well as uploaded as a local build artifact.
The base image is uploaded to the Image directory to allow us to hopefully shortcut as much of the Third-Party build process as possible on the next build.
While we could also use that image as the base for our other builds, another Third-Party build at around the same time could
replace the workflow's image with a new image.
To counter that, the base image is loaded as a local build artifact, with that artifact being removed at the final stage.
As the local build artifact is scoped to the workflow, it can be used with the confidence that it was produce by the `Third-Party` build in the current workflow.

## `Lint` Job

This job exists to apply the [Pre-Commit](https://pre-commit.com/) workflow to the entire project.
With more information in the [pre-commit document](../../pre-commit.md), this job attempts to ensure that any agreed on requirements and guidelines are applied uniformly to our repository.

### Lint Steps

The Pre-Commit program is implemented in Python and a step is present to install the application's package.
The next two steps are present to allow caching of the environments used by the Pre-Commit application to scan the repository.
Those two steps ensure that the last step, the actual invocation of the Pre-Commit application, can be executed as efficiently as possible.

## Build Jobs

Each build job is present in the workflow to either build a specific flavor of the project for a given reason or to ensure that we are testing a specific flavor of the project for a given reason.

| Build | Description |
|---|---|
| Core | Quickest build configuration, ensure core components compile cleanly and unit tests complete. |
|SDK | Build a Release version of the product's SDK. |
| Debug_Core | Version of the `Core` build with debug enabled to... [TBD] |
| LLMV_Tests | Version of the build that specifically targets LLVM and GaiaT related tests. |

### Docker Setup And Pulling the Dev-Base Image

After the initial setup tasks, the next task for each of the build jobs is to setup for interaction with Docker.
This is required to pull the latest version from GitHub's image store:

```YAML
      - name: Pull Latest Dev-Core Image
        run: |
          ssh-agent -a $SSH_AUTH_SOCK > /dev/null
          echo "${{ secrets.GITHUB_TOKEN }}" | docker login ghcr.io -u ${{ github.actor }} --password-stdin
          docker pull ghcr.io/gaia-platform/dev-base:latest
```

By setting things up this way, we can leverage the work done in the `gdev` project to build images.
This leveraging has an added side effect of being portable from a build machine to a developer machine.

#### References

- [Using a SSH deploy key in GitHub Actions to access private repositories](https://www.webfactory.de/blog/use-ssh-key-for-private-repositories-in-github-actions)
  - Note that we do not need the second line of the example, as we are currently logging in using the github actor:
    ```sh
    ssh-add - <<< "${{ secrets.SSH_PRIVATE_KEY }}"
    ```
- [Working with the Container registry](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry)
- [Publishing and installing a package with GitHub Actions](https://docs.github.com/en/packages/managing-github-packages-using-github-actions-workflows/publishing-and-installing-a-package-with-github-actions#upgrading-a-workflow-that-accesses-ghcrio)

### Building The Docker Image

The actual command to build the docker image is very long.
To avoid as many mistakes as possible, most of the invocation logic is encapsulated within the `build_image.sh` script.
The first two parameter rarely change, pointing to where the source code is and the base image to use for thw new image.
There are optional parameters after those two parameters that allow the specification of configuration parameters that are passed directly into `gdev`.
These `--cfg-enables` parameters are passed directly into `gdev` without modification.

```YAML
      - name: Build Docker Image
        run: |
          $GAIA_REPO/dev_tools/github-actions/build_image.sh \
              --repo-path $GAIA_REPO \
              --base-image $DEV_IMAGE \
              --cfg-enables GaiaSDK
```

### Post Build Actions

Post build actions are used to perform some manner of action on the build image.
As docker build images are just files, the images must first be stood up in a docker container before they can be interacted with.
Like how the `build_image.sh` script hides some complicated `docker build` invocations, the `post_build_action.sh` script hides the invocations necessary to stand up an image and to execute the specified action within that image.

The two currently available actions are:

- `unit_tests`
  - run all known unit tests against the project
- `publish_package`
  - take the packaged project and make it available as an artifact

```YAML
      - name: Run Unit Tests
        run: |
          $GAIA_REPO/dev_tools/github-actions/post_build_action.sh \
            --repo-path $GAIA_REPO \
            --gaia-version $GAIA_VERSION \
            --action unit_tests
```

## Test Jobs

Tests jobs are usually pretty straight forward.
Like the use of scripts for Build Jobs, the `execute_tests_against_package.sh` script keys off information supplied on its command line to execute a specific set of tests.
Utilizing the Debian Installl Package from the `Download Debian Install Package` step, the script also makes sure that the new package is installed in the current GHA container before the test script is called.

```YAML
      - name: Download Debian Install Package
        uses: actions/download-artifact@v2
        with:
          name: SDK Install Package
          path: ${{ github.workspace }}/production/tests/packages

      - name: Tests
        working-directory: ${{ github.workspace }}
        run: |
          $GAIA_REPO/dev_tools/github-actions/execute_tests_against_package.sh --job-name $GITHUB_JOB --repo-path $GAIA_REPO --package $GAIA_REPO/production/tests/packages
```
