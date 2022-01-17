# GitHub Actions

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

## Information Relevant to All(Most Steps)

The `Lint` job is special, and is covered in its own section below.
For the other sections, there is a lot of overlap.

### Build System

Each of our jobs is built on a standard Ubuntu:20.04 system.
Any specialization of jobs for different environments is done within the docker builds.

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
  SLACK_STATUS_FIELDS: repo,message,commit,author,action,eventName,ref,workflow,job,took,pullRequest
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
| `SLACK_STATUS_FIELDS` | job fields to broadcast to slack |

### Steps Prefix

For each job, two prerequisites are required: the repository is available and Python3.8 is installed to work against that repository.
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

### Docker Setup And Pulling the Dev-Base Image

The next task for each of the jobs is to setup for interaction with Docker and to pull the latest version from GitHub's image store:

```YAML
      - name: Pull Latest Dev-Core Image
        run: |
          ssh-agent -a $SSH_AUTH_SOCK > /dev/null
          echo "${{ secrets.GITHUB_TOKEN }}" | docker login ghcr.io -u ${{ github.actor }} --password-stdin
          docker pull ghcr.io/gaia-platform/dev-base:latest
```


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

### XXX

Because of the previously stated reasons, the Dockerfile that is generated by the gdev framework is slightly modified:

```sh
$GAIA_REPO/dev_tools/gdev/gdev.sh dockerfile > $GAIA_REPO/production/raw_dockerfile
$GAIA_REPO/dev_tools/github-actions/copy_until_line.py --input-file $GAIA_REPO/production/raw_dockerfile --output-file $GAIA_REPO/production/dockerfile --stop-line "# GenRunDockerfile(production)"
```

## `Lint` Job

TBD

## Build Jobs

## Test Jobs


# We do not need this as we are currently logging in using the github actor.
#         ssh-add - <<< "${{ secrets.SSH_PRIVATE_KEY }}"

#         https://www.webfactory.de/blog/use-ssh-key-for-private-repositories-in-github-actions
#         If we do need to export, https://github.com/webfactory/ssh-agent might be better.
#         Combined with https://github.com/marketplace/actions/install-ssh-key ?


#         https://docs.github.com/en/packages/managing-github-packages-using-github-actions-workflows/publishing-and-installing-a-package-with-github-actions#upgrading-a-workflow-that-accesses-ghcrio


#         https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry
