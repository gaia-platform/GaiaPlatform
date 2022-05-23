#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide high level scenario tests for the Docker_Dev project.
"""

from test.gdev_execute import (
    determine_old_script_behavior,
    determine_repository_production_directory,
    SubprocessExecutor,
    determine_repository_base_directory,
)
from gdev.host import Host


def get_executor():
    """
    Get the executor to use for invoking the Gdev application for testing.
    """
    return SubprocessExecutor()


def find_drydock_line(expected_output, drydock_prefix):
    """
    Find a drydock line with the specified prefix in the output.
    """

    assert drydock_prefix in expected_output
    start_index = expected_output.index(drydock_prefix)
    end_index = expected_output.index("]\n", start_index)
    last_line = expected_output[start_index + len(drydock_prefix) : end_index]
    return last_line


def __find_docker_tag_line(expected_output):

    tag_line = find_drydock_line(expected_output, "[execute:docker tag ")
    print(f"tag_line:{tag_line}")
    return tag_line


def __find_docker_push_line(expected_output):

    push_line = find_drydock_line(expected_output, "[execute:docker push ")
    print(f"push_line:{push_line}")
    return push_line


def find_docker_build_line(expected_output):
    """
    Find the drydock line corresponding to a docker build.
    """

    build_line = find_drydock_line(expected_output, "[execute:docker buildx build ")
    print(f"build_line:{build_line}")
    return build_line


def find_and_remove(
    line_to_look_in,
    part_to_look_for,
    look_at_end=False,
    search_for_end_whitespace=False,
    replace_with=None,
):
    """
    Find a given sequence, with respect to whitespace, and replace it with a given value.
    """

    if look_at_end:
        assert line_to_look_in.endswith(part_to_look_for)
        line_to_look_in = line_to_look_in[0 : -(len(part_to_look_for))]
    else:
        part_index = line_to_look_in.index(part_to_look_for)
        if search_for_end_whitespace:
            end_index = line_to_look_in.index(" ", part_index + len(part_to_look_for))
        else:
            end_index = part_index + len(part_to_look_for)
        line_to_look_in = (
            line_to_look_in[0:part_index]
            + (replace_with if replace_with else "")
            + line_to_look_in[end_index:]
        )
    return line_to_look_in


def construct_base_build_line(is_using_mixins=False):
    """
    Construct our understanding of what the build line should be.
    """

    current_hash = Host.execute_and_get_line_sync("git rev-parse HEAD")

    run_type = "custom" if is_using_mixins else "run"
    return (
        f"-f {determine_repository_base_directory()}/.gdev/production"
        + f"/{run_type}.dockerfile.gdev "
        + f'-t production__{run_type}:latest --label GitHash="{current_hash}" '
        + "--build-arg BUILDKIT_INLINE_CACHE=1 --platform linux/amd64 --shm-size 1gb "
        + f"--ssh default  {determine_repository_base_directory()}"
    )


def __construct_base_tag_command_line(is_using_mixins=False):
    run_type = "custom" if is_using_mixins else "run"
    return f"production__run:latest None/production__{run_type}:latest"


def __construct_base_push_command_line(is_using_mixins=False):
    run_type = "custom" if is_using_mixins else "run"
    return f"None/production__{run_type}:latest"


def test_show_cfg():
    """
    Make sure that show information about the configuration file in the current directory.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["cfg"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        '# enable by setting "Debug"' in expected_output
    ), "Original output missing config hint."
    assert (
        '# enable by setting "GaiaLLVMTests"' in expected_output
    ), "Original output missing config hint."


def test_show_cfg_with_cfg_enable_debug():
    """
    Make sure that show information about the configuration file in the current directory,
    with the setting of --cfg-enable Debug
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["cfg", "--cfg-enable", "Debug"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        '# enable by setting "Debug"' not in expected_output
    ), "Original output contains config hint."
    assert (
        '# enable by setting "GaiaLLVMTests"' in expected_output
    ), "Original output missing config hint."


def test_show_cfg_with_cfg_enable_debug_and_llvm():
    """
    Make sure that show information about the configuration file in the current directory,
    with the setting of --cfg-enable Debug GaiaLLVMTests
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["cfg", "--cfg-enable", "Debug", "GaiaLLVMTests"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        '# enable by setting "Debug"' not in expected_output
    ), "Original output contains config hint."
    assert (
        '# enable by setting "GaiaLLVMTests"' not in expected_output
    ), "Original output missing config hint."


def test_generate_dockerfile():
    """
    Make sure that we can generate a dockerfile from the current directory.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--backward"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        "-DCMAKE_BUILD_TYPE=Debug" not in expected_output
    ), "Original output contains untriggered line."
    assert (
        "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output
    ), "Original output contains untriggered line."


def test_generate_new_dockerfile():
    """
    Make sure that we can generate a dockerfile from the current directory.
    """

    # Arrange
    executor = get_executor()
    base = determine_repository_base_directory()

    supplied_arguments = ["dockerfile"]
    expected_return_code = 0
    expected_output = (
        f"Dockerfile written to: {base}/.gdev/production/run.dockerfile.gdev"
    )
    expected_error = (
        f"(production) Creating dockerfile {base}/.gdev/production/run.dockerfile.gdev"
    )

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_generate_dockerfile_debug():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug

    Note that the `cfg-enable` flag influences the creation of the dockerfile,
    but will not be visible in anything other than the dockerfile.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--backward", "--cfg-enable", "Debug"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        "-DCMAKE_BUILD_TYPE=Debug" in expected_output
    ), "Original output does not contain triggered line."
    assert (
        "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output
    ), "Original output contains untriggered line."


def test_generate_dockerfile_debug_and_llvm():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug GaiaLLVMTests

    Note that the `cfg-enable` flag influences the creation of the dockerfile,
    but will not be visible in anything other than the dockerfile.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = [
        "dockerfile",
        "--backward",
        "--cfg-enable",
        "Debug",
        "GaiaLLVMTests",
    ]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        "-DCMAKE_BUILD_TYPE=Debug" in expected_output
    ), "Original output does not contain triggered line."
    assert (
        "-DBUILD_GAIA_LLVM_TESTS=ON" in expected_output
    ), "Original output does not contain triggered line."


def test_generate_dockerfile_debug_and_new_base_image():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug --base-image frogger

    Note that the `base-image` flag influences the creation of the dockerfile,
    but will not be visible in anything other than the dockerfile.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = [
        "dockerfile",
        "--backward",
        "--cfg-enable",
        "Debug",
        "--base-image",
        "frogger",
    ]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert (
        "frogger" in expected_output
    ), "Original output does not contain specified base image."
    assert (
        "ubuntu:20.04" not in expected_output
    ), "Original output contains default base image."
    assert "RUN groupadd -r -o -g 1000" not in expected_output


def test_generate_dockerfile_mixins_sshd():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --mixins sshd
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--backward", "--mixins", "sshd"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "RUN groupadd -r -o -g 1000" not in expected_output
    assert "openssh-server" in expected_output


def test_generate_dockerfile_mixins_sshd_and_nano():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --mixins sshd nano
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--backward", "--mixins", "sshd", "nano"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "RUN groupadd -r -o -g 1000" not in expected_output
    assert "openssh-server" in expected_output
    assert "nano" in expected_output


def test_generate_docker_build():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile.  Note that this uses the `--drydock` test argument to simulate
    calling docker without actually calling docker.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--dry-dock"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()


def test_generate_docker_build_with_platform():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a specified platform.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--dry-dock", "--platform", "arm64"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(
        build_line, " --platform linux/arm64", replace_with=" --platform linux/amd64"
    )
    assert build_line == construct_base_build_line()


def test_generate_docker_build_with_registry():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a specified registry.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--dry-dock", "--registry", "localhost"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(
        build_line,
        " --cache-from localhost/production__",
        search_for_end_whitespace=True,
        replace_with=" ",
    )
    assert build_line == construct_base_build_line()


def test_generate_docker_build_with_mixins_sudo():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a single mixin.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--dry-dock", "--mixins", "sudo"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(build_line, " --label Mixins=\"['sudo']\"")
    assert build_line == construct_base_build_line(is_using_mixins=True)


def test_generate_docker_build_with_mixins_sudo_and_nano():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a couple of mixins.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--dry-dock", "--mixins", "sudo", "nano"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(build_line, " --label Mixins=\"['nano','sudo']\"")
    assert build_line == construct_base_build_line(is_using_mixins=True)


def test_generate_docker_push():
    """
    Make sure that we can generate a request to docker to push the image built
    by previous steps.  Note that this uses the `--drydock` test argument to simulate
    calling docker without actually calling docker.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["push", "--backward", "--dry-dock"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    tag_line = __find_docker_tag_line(expected_output)
    assert tag_line == __construct_base_tag_command_line()

    push_line = __find_docker_push_line(expected_output)
    assert push_line == __construct_base_push_command_line()


def test_generate_docker_push_new():
    """
    Make sure that we can generate a request to docker to push the image built
    by previous steps.  Note the new version catches a registry not being
    specified and stops the program.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["push", "--dry-dock"]
    expected_return_code = 1
    expected_output = ""
    expected_error = (
        "Error: The --registry arguments must specify the registry to push to."
    )

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
