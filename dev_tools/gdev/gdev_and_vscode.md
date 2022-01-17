# `gdev` and VS Code
VS Code can attach to running `gdev` Docker containers and run the VS Code Server and extensions inside a container, instead of running them locally (and only interacting with the container through the terminal). This improves IntelliSense by making all files imported through dependencies available for VS Code to read.

## I already did this, give me the TL;DR
These steps are for developers who already set everything up and just want to remember how to start VS Code + `gdev` up again.

- In a terminal (not in VS Code):
    ```bash
    cd GaiaPlatform/production
    gdev run --mixins git
    ```
- Leave that terminal open and open VS Code.
- Press `F1`, select **Remote-Containers: Attach to Running Container...** and pick the `gdev` container (it will have a random name).
  - If it shows a popup error about not finding a running container, select **Choose Container**.
- *Develop in VS Code as usual...*
- Don't type `exit` into the terminal with `gdev run` in it until you close VS Code.

Attached container configurations can be edited with `F1` -> **Remote-Containers: Open Container Configuration File**.


## Get started
Open a terminal (not in VS Code) and run:
```bash
cd GaiaPlatform/production
gdev run --mixins git
```
You can add any of your other `gdev run` flags, but we need `git` in there so the VS Code Server can use it (your `git` credentials are forwarded to the container so you can run `git` commands inside).

Keep this terminal open for now.

Open VS Code and install the [Remote - Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension. In the Command Palette (`F1`), run **Remote-Containers: Attach to Running Container...** and select the container that `gdev run` just started.

We need to configure how we attach to these containers in the future. In the Command Palette, run **Remote-Containers: Open Container Configuration File** and copy-paste the following into the JSON file that opens:
```javascript
{
    // Default path to open when attaching to a new container.
    "workspaceFolder": "/source",
  
    // An array of extension IDs that specify the extensions to
    // install inside the container when you first attach to it.
    "extensions": [
        "ms-azuretools.vscode-docker",
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "twxs.cmake"
    ],
}
```

Add your other extensions to the `extensions` list. You can find more settings in the [Attached container configuration reference](https://code.visualstudio.com/docs/remote/devcontainerjson-reference#_attached-container-configuration-reference). We are only using the "attached container" subset of **Remote - Containers** features instead of the [devcontainer](https://code.visualstudio.com/docs/remote/containers) features because `gdev` uses BuildKit steps that devcontainers do not yet support. Devcontainers expose more [configuration options](https://code.visualstudio.com/docs/remote/devcontainerjson-reference) for their containers, but the **Remote - Containers** extension handles building and running them (instead of the user).

Attached container configuration files are located in `~/.config/Code/User/globalStorage/ms-vscode-remote.remote-containers/imageConfigs` (locally, not in containers). These files are automatically named after the images they were attached to. When VS Code attaches to a Docker container, it auto-selects the configuration file that matches the container image's name.

Close VS Code. Go back to the terminal with `gdev run` in it and exit then re-run the container:
```bash
# Container is running
exit
# Container was stopped, now you are back in your local shell
gdev run --mixins git
```

Keep this terminal open while you work in VS Code or else the container will stop.

Open VS Code again. It will warn you that it cannot find the previous container; select **Choose Container** and select the newly running container (it has a randomized name). Now it will attach to that container in the `/source` directory and install VS Code extensions into the container.

## C++ IntelliSense
VS Code's C++ Intellisense needs to know where to look for files that come from dependencies installed in the container (anything listed for installation in `gdev.cfg` files). We need to list the appropriate include paths.

If you don't already have the `GaiaPlatform/.vscode/c_cpp_properties.json` file, create it. You can also open the Command Palette (`F1`) and select **C/C++: Edit Configurations (UI/JSON)**.

Here is a sample configuration:
```javascript
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/third_party/production/TranslationEngineLLVM/clang/include",
                "/build/production/deps/flatbuffers/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/clang++-13",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "${default}",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

Whenever IntelliSense fails to find a file, add its include directory to `includePath`.

- Dependencies such as `flatbuffers` have their include files in the `/build` directory.
- While `${workspaceFolder}/**` recursively searches all include paths in the repo, directories such as `TranslationEngineLLVM` may contain too many files for the searcher and will require additional include paths.
