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
    assert '# enable by setting "Debug"' in expected_output, "Original output missing config hint."
    assert '# enable by setting "GaiaLLVMTests"' in expected_output, "Original output missing config hint."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

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
    assert '# enable by setting "Debug"' not in expected_output, "Original output contains config hint."
    assert '# enable by setting "GaiaLLVMTests"' in expected_output, "Original output missing config hint."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

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
    assert '# enable by setting "Debug"' not in expected_output, "Original output contains config hint."
    assert '# enable by setting "GaiaLLVMTests"' not in expected_output, "Original output missing config hint."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_dockerfile():
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
    assert "-DCMAKE_BUILD_TYPE=Debug" not in expected_output, "Original output contains untriggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output, "Original output contains untriggered line."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_dockerfile_debug():
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
    assert "-DCMAKE_BUILD_TYPE=Debug" in expected_output, "Original output does not contain triggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" not in expected_output, "Original output contains untriggered line."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_dockerfile_debug_and_llvm():
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
    assert "-DCMAKE_BUILD_TYPE=Debug" in expected_output, "Original output does not contain triggered line."
    assert "-DBUILD_GAIA_LLVM_TESTS=ON" in expected_output, "Original output does not contain triggered line."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

def test_show_dockerfile_debug_and_new_base_image():
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
    assert "frogger" in expected_output, "Original output does not contain specified base image."
    assert "ubuntu:20.04" not in expected_output, "Original output contains default base image."

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
