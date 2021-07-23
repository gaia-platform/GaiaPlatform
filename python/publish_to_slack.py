#! /usr/bin/python3

"""
Script to publish the supplied information to the #test channel on Slack.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json
import sys
import argparse
import os
import requests

# https://api.slack.com/tools/block-kit-builder

TEST_SLACK_CHANNEL = "#test"
TEST_USER_NAME = "TestMonitor"
TEST_BOT_TOKEN = "xoxb-698692788439-2318692817248-wfMSwDES7KZjqak76X0QU57O"
TEST_CHANNEL_URL = (
    "https://hooks.slack.com/services/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"
)


def process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description=f"Publish test information to the {TEST_SLACK_CHANNEL} channel."
    )
    subparsers = parser.add_subparsers()
    messages = subparsers.add_parser(
        "message", aliases=["m"], help="Post a message to the slack channel."
    )
    messages.set_defaults(which="message")
    messages.add_argument("title", help="Title to display with the message.")
    messages.add_argument("message", help="Message to display in the channel.")
    attachments = subparsers.add_parser(
        "attachment", aliases=["a"], help="Post an attachment to the slack channel."
    )
    attachments.set_defaults(which="attachment")
    attachments.add_argument("filename", help="Name of the attachment file.")
    attachments.add_argument("filetype", help="MIME-type of the attachment file.")

    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)
    return parser.parse_args()


def post_message(title, message):
    """
    Post a simple message to the slack channel.
    """

    slack_data = {
        "username": TEST_USER_NAME,
        "channel": TEST_SLACK_CHANNEL,
        "attachments": [
            {
                "color": "#9733EE",
                "fields": [
                    {
                        "title": title,
                        "value": message,
                        "short": "false",
                    }
                ],
            }
        ],
    }
    request_headers = {
        "Content-Type": "application/json",
        "Content-Length": str(sys.getsizeof(slack_data)),
    }
    return requests.post(
        TEST_CHANNEL_URL, data=json.dumps(slack_data), headers=request_headers
    )


def post_attachment(filename, file_type):
    """
    Post the specific file to the slack channel.
    """

    if not os.path.exists(filename):
        print("Attachment file '{filename}' does not exist.")
        sys.exit(1)
    if not os.path.isfile(filename):
        print("Attachment file '{filename}' is not a file.")
        sys.exit(1)

    with open(filename, "rb") as attachment_file:
        attachment_file_object = {
            "file": (filename, attachment_file, file_type, {"Expires": "0"})
        }
        slack_data = {
            "token": TEST_BOT_TOKEN,
            "channels": TEST_SLACK_CHANNEL,
            "media": attachment_file_object,
        }
        response = requests.post(
            url="https://slack.com/api/files.upload",
            data=slack_data,
            headers={"Accept": "application/json"},
            files=attachment_file_object,
        )
    return response


args = process_command_line()
if args.which == "message":
    post_response = post_message(args.title, args.message)
else:
    assert args.which == "attachment"
    post_response = post_attachment(args.filename, args.filetype)

EXIT_CODE = 0
if not post_response.ok:
    print(post_response.text)
    EXIT_CODE = 1
sys.exit(EXIT_CODE)
