#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide high level scenario tests for the Docker_Dev project.
"""

from test.gdev_execute import (
    determine_old_script_behavior,
    determine_repository_production_directory,
    SubprocessExecutor,
)


def get_executor():
    """
    Get the executor to use for invoking the Gdev application for testing.
    """
    return SubprocessExecutor()


def test_show_help_x():
    """
    Make sure that we can show help about the various things to do.
    """

    # Arrange
    executor = get_executor()
    suppplied_arguments = ["--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)
    expected_output = expected_output.replace(
        "\nGaiaPlatform build and development environment tool.\n", ""
    )

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `build` subcommand entry point.1",
    )
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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Parse gdev.cfg for build rules.",
        "Class to provide for the `cfg` subcommand entry point.1",
    )
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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `dockerfile` subcommand entry point.1",
    )
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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)
    expected_output = expected_output.replace(
        "\nInternal component commands for top-level "
        + "gdev commands. These should rarely\n"
        + "be needed.\n",
        "",
    )

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `push` subcommand entry point.1",
    )
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
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(suppplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=suppplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `run` subcommand entry point.1",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
