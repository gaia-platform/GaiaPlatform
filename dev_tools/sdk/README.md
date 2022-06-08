# Release Process

**NB: This process may be outdated.**

When we are ready to release a new version of Gaia this is the process to follow:

1. Ensure you are on `master` and have the latest changed:
   ```shell
   git checkout master
   git pull
   ```
2. Create a branch for the version to release:
   ```shell
    git checkout -b gaia-release-0.3.0-beta
    ```
3. Bump the project version in the [production/CMakeLists.txt](../../production/CMakeLists.txt) according to Semantic Versioning 2.0 spec. Note that Major version bumps should involve consultation with a number of folks across the team.
   ```cmake
   # From
   project(production VERSION 0.2.5)
   # To
   project(production VERSION 0.3.0)
   ```
4. Change, if necessary, the `PRE_RELEASE_IDENTIFIER` in the [production/CMakeLists.txt](../../production/CMakeLists.txt). For GA releases leave the `PRE_RELEASE_IDENTIFIER` empty.
   ```cmake
   # From
   set(PRE_RELEASE_IDENTIFIER "alpha")
   # To
   set(PRE_RELEASE_IDENTIFIER "beta")
   ```
5. Create a commit for the new Release:
   ```shell
   git add -u
   git commit -m "Bump Gaia version to 0.3.0-beta"
   git push --set-upstream origin gaia-release-0.3.0-beta
   # Create a PR to push the change into master.
   ```
6. Create a tag reflecting the new version:
   ```shell
   # Pull the version change after the PR is approved and merged.
   git checkout master
   git pull
   # Create and push the version tag.
   git tag v0.3.0-beta
   git push origin v0.3.0-beta
   ```
7. Go on [GitHub releases tab](https://github.com/gaia-platform/GaiaPlatform/releases) and draft a new release, using the tag created in the previous step.
    1. Tag Version: `0.3.0-beta`
    2. Release Title: `Gaia Platform 0.3.0-beta`
    3. Description: High level description of new features and relevant bug fixes.
    4. Check the box "This is a pre-release" if that's the case.
    5. Download the `.deb` and `.rpm` from TeamCity and attach them to the release.
8. From now on the version will remain `0.3.0-beta` until a new Release is ready. At that point repeat this process.
    1. We currently have a single version across the product (gaia_sdk, gaia_db_server, gaiac, and gaiat).
    2. Every build has an incremental build number which is added to the full version string (eg. `0.2.1-alpha+1731`). The build number may differ for local builds.
