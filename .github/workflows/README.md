# GitHub Actions

This directory contains YAML files that are used by [GitHub Actions](https://docs.github.com/en/actions).
For this project, any files in this directory are generated using the scripts in [this subproject](https://github.com/gaia-platform/gaia-platform/blob/master/dev_tools/github-actions/README.md).

**PLEASE DO NOT MODIFY THESE FILES DIRECTLY**

## Design

tp, then lint, then others
tp minimizes the time spent building other docker images
lint leverages tp to 

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

These are set in each job to ensure a stable environment is available:

```YAML
    env:
      GAIA_REPO: ${{ github.workspace }}
      SSH_AUTH_SOCK: /tmp/ssh_agent.sock
```

| Name | Description |
| --- | --- |
| `GAIA_REPO` | where the workspace is within the workspace provided by GHA |
| `SSH_AUTH_SOCK` | needed for docker related commands issued within the scripts |

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

### Docker Setup And Pulling the Dev-Core Image

```YAML
      - name: Pull Latest Dev-Core Image
        run: |
          ssh-agent -a $SSH_AUTH_SOCK > /dev/null
          echo "${{ secrets.GITHUB_TOKEN }}" | docker login ghcr.io -u ${{ github.actor }} --password-stdin
          docker pull ghcr.io/gaia-platform/dev-base:latest
```
## `Third-Party` Job



# We do not need this as we are currently logging in using the github actor.
#         ssh-add - <<< "${{ secrets.SSH_PRIVATE_KEY }}"

#         https://www.webfactory.de/blog/use-ssh-key-for-private-repositories-in-github-actions
#         If we do need to export, https://github.com/webfactory/ssh-agent might be better.
#         Combined with https://github.com/marketplace/actions/install-ssh-key ?


#         https://docs.github.com/en/packages/managing-github-packages-using-github-actions-workflows/publishing-and-installing-a-package-with-github-actions#upgrading-a-workflow-that-accesses-ghcrio
