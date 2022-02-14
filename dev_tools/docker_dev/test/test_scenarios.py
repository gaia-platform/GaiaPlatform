from test.gdev_execute import MainlineExecutor, determine_old_script_behavior, determine_repository_production_directory

def test_show_help():
    """
    Make sure that we can show help about the various things to do.
    """

    # Arrange
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
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
    executor = MainlineExecutor()
    suppplied_arguments = ["cfg"]
    expected_return_code, expected_output, expected_error = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=determine_repository_production_directory())

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
