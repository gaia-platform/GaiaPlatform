from test.gdev_execute import MainlineExecutor, determine_old_script_behavior, determine_repository_production_directory, SubprocessExecutor

def get_executor():
    return SubprocessExecutor()

def test_show_help_x():
    """
    Make sure that we can show help about the various things to do.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_help_build():
    """
    Make sure that we can show help about the build task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["build", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    expected_output = expected_output.replace("Dependency(options: 'Options')", "Bob")
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_help_cfg():
    """
    Make sure that we can show help about the cfg task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["cfg", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    expected_output = expected_output.replace("Parse gdev.cfg for build rules.", "Parse the target gdev.cfg for build rules.")
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
def test_show_help_dockerfile():
    """
    Make sure that we can show help about the dockerfile task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    expected_output = expected_output.replace("Dependency(options: 'Options')", "Class to encapsulate the 'cfg' subcommand.")
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
def test_show_help_gen():
    """
    Make sure that we can show help about the gen task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["gen", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_help_push():
    """
    Make sure that we can show help about the push task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["push", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    expected_output = expected_output.replace("Dependency(options: 'Options')", "Bob")
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_help_run():
    """
    Make sure that we can show help about the run task.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["run", "--help"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    expected_output = expected_output.replace("Dependency(options: 'Options')", "Bob")
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_cfg():
    """
    Make sure that show information about the configuration file in the current directory.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["cfg"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert '# enable by setting "Debug"' in expected_output, "Original output missing config hint."
    assert '# enable by setting "GaiaLLVMTests"' in expected_output, "Original output missing config hint."

def test_show_cfg_with_cfg_enable_debug():
    """
    Make sure that show information about the configuration file in the current directory,
    with the setting of --cfg-enable Debug
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["cfg", "--cfg-enable", "Debug"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert '# enable by setting "Debug"' not in expected_output, "Original output contains config hint."
    assert '# enable by setting "GaiaLLVMTests"' in expected_output, "Original output missing config hint."

def test_show_cfg_with_cfg_enable_debug_and_llvm():
    """
    Make sure that show information about the configuration file in the current directory,
    with the setting of --cfg-enable Debug GaiaLLVMTests
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["cfg", "--cfg-enable", "Debug", "GaiaLLVMTests"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert '# enable by setting "Debug"' not in expected_output, "Original output contains config hint."
    assert '# enable by setting "GaiaLLVMTests"' not in expected_output, "Original output missing config hint."

def test_generate_dockerfile():
    """
    Make sure that we can generate a dockerfile from the current directory.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "-DCMAKE_BUILD_TYPE=Debug" not in expected_output, "Original output contains untriggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output, "Original output contains untriggered line."

def test_generate_dockerfile_debug():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile", "--cfg-enable", "Debug"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "-DCMAKE_BUILD_TYPE=Debug" in expected_output, "Original output does not contain triggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output, "Original output contains untriggered line."

def test_generate_dockerfile_debug_and_llvm():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug GaiaLLVMTests
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile", "--cfg-enable", "Debug", "GaiaLLVMTests"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "-DCMAKE_BUILD_TYPE=Debug" in expected_output, "Original output does not contain triggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" in expected_output, "Original output does not contain triggered line."

def test_generate_dockerfile_debug_and_new_base_image():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --cfg-enable Debug --base-image frogger
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile", "--cfg-enable", "Debug", "--base-image", "frogger"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "frogger" in expected_output, "Original output does not contain specified base image."
    assert "ubuntu:20.04" not in expected_output, "Original output contains default base image."
    assert "RUN groupadd -r -o -g 1000" not in expected_output

def test_generate_dockerfile_mixins_sshd():
    """
    Make sure that we can generate a dockerfile from the current directory,
        with the setting of --mixins sshd
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["dockerfile", "--mixins", "sshd"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

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
    suppplied_arguments = ["dockerfile", "--mixins", "sshd", "nano"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

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
    suppplied_arguments = ["build", "--dry-dock"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "--platform linux/amd64" in expected_output

def test_generate_docker_build_with_platform():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a specified platform.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["build", "--dry-dock", "--platform", "arm64"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "--platform linux/arm64" in expected_output

def test_generate_docker_build_with_registry():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a specified registry.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["build", "--dry-dock", "--registry", "localhost"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert "--cache-from localhost/production__" in expected_output

def test_generate_docker_build_with_mixins_sudo():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a single mixin.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["build", "--dry-dock", "--mixins", "sudo"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert '--label Mixins="[\'sudo\']"' in expected_output

def test_generate_docker_build_with_mixins_sudo_and_nano():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile with a couple of mixins.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["build", "--dry-dock", "--mixins", "sudo", "nano"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    assert '--label Mixins="[\'nano\',\'sudo\']"' in expected_output

def __find_docker_run_line(expected_output):

    run_prefix = "[execvpe:docker run "
    assert run_prefix in expected_output
    start_index = expected_output.index(run_prefix)
    end_index = expected_output.index("]\n", start_index)
    last_line = expected_output[start_index+len(run_prefix):end_index]
    return last_line

def test_generate_docker_run():
    """
    Make sure that we can generate a request to docker to build the image created
    by the dockerfile.  Note that this uses the `--drydock` test argument to simulate
    calling docker without actually calling docker.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["run", "--dry-dock"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
    last_line = __find_docker_run_line(expected_output)
    print(">>" + last_line)
    assert False
